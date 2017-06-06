#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Component.hpp"
#include <concurrent_queue.h>
#include <ppl.h>
#include <map>

namespace Gek
{
    namespace Implementation
    {
        class Entity
            : public Edit::Entity
        {
        private:
            ComponentMap componentMap;

        public:
            void addComponent(Plugin::Component *component, std::unique_ptr<Plugin::Component::Data> &&data)
            {
                componentMap[component->getIdentifier()] = std::move(data);
            }

            void removeComponent(const std::type_index &type)
            {
                auto componentSearch = componentMap.find(type);
                if (componentSearch != std::end(componentMap))
                {
                    componentMap.erase(componentSearch);
                }
            }

            void listComponents(std::function<void(const std::type_index &, const Plugin::Component::Data *)> onComponent)
            {
                for (const auto &component : componentMap)
                {
                    onComponent(component.first, component.second.get());
                }
            }

            // Edit::Entity
            ComponentMap &getComponentMap(void)
            {
                return componentMap;
            }

            // Plugin::Entity
            bool hasComponent(const std::type_index &type) const
            {
                return (componentMap.count(type) > 0);
            }

			Plugin::Component::Data *getComponent(const std::type_index &type)
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == std::end(componentMap))
				{
                    return nullptr;
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(const std::type_index &type) const
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == std::end(componentMap))
				{
                    return nullptr;
                }

				return componentSearch->second.get();
			}
		};

        GEK_CONTEXT_USER(Population, Plugin::Core *)
            , public Edit::Population
        {
        private:
            Plugin::Core *core = nullptr;

            ShuntingYard shuntingYard;
            concurrency::concurrent_queue<Action> actionQueue;

            std::unordered_map<std::string, std::type_index> componentTypeNameMap;
            std::unordered_map<std::type_index, std::string> componentNameTypeMap;
            ComponentMap componentMap;

            ThreadPool loadPool;
            concurrency::concurrent_queue<std::function<void(void)>> entityQueue;
            EntityMap entityMap;

            uint32_t uniqueEntityIdentifier = 0;

        public:
            Population(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , loadPool(1)
            {
                assert(core);

                std::cout << "Loading component plugins" << std::endl;
                getContext()->listTypes("ComponentType", [&](std::string const &className) -> void
                {
                    std::cout << "Component found: " << className << std::endl;
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    if (componentMap.count(component->getIdentifier()) > 0)
                    {
                        std::cerr << "Duplicate component identifier found: Class(" << className << "), Identifier(" << component->getIdentifier().name() << ")" << std::endl;
                        return;
                    }

                    if (componentNameTypeMap.count(component->getIdentifier()) > 0)
                    {
                        std::cerr << "Duplicate component name found: Class(" << className << "), Name(" << component->getName() << ")" << std::endl;
                        return;
                    }

                    componentNameTypeMap.insert(std::make_pair(component->getIdentifier(), component->getName()));
                    componentTypeNameMap.insert(std::make_pair(component->getName(), component->getIdentifier()));
                    componentMap[component->getIdentifier()] = std::move(component);
                });

                core->onExit.connect<Population, &Population::onExit>(this);
            }

            ~Population(void)
            {
				loadPool.drain();

                core->onExit.disconnect<Population, &Population::onExit>(this);

                entityMap.clear();
                componentTypeNameMap.clear();
                componentMap.clear();
            }

            void queueEntity(Plugin::Entity *entity, std::string const & requestedName)
            {
                entityQueue.push([this, entity = entity, requestedName = requestedName](void) -> void
                {
                    auto entityName(requestedName.empty() ? String::Format("unnamed_%v", ++uniqueEntityIdentifier) : requestedName);
                    if (entityMap.count(requestedName) > 0)
                    {
                        std::cerr << "Unable to add entity to scene: " << entityName << std::endl;
                    }
                    else
                    {
                        entityMap[entityName].reset(entity);
                        onEntityCreated.emit(entity, entityName);
                    }
                });
            }

            // Core
            void onExit(void)
            {
                loadPool.reset();
            }

            // Edit::Population
            ComponentMap &getComponentMap(void)
            {
                return componentMap;
            }

            EntityMap &getEntityMap(void)
            {
                return entityMap;
            }

            Edit::Component *getComponent(const std::type_index &type)
            {
                auto componentsSearch = componentMap.find(type);
                if (componentsSearch != std::end(componentMap))
                {
                    return dynamic_cast<Edit::Component *>(componentsSearch->second.get());
                }
                
                return nullptr;
            }

            // Plugin::Population
            ShuntingYard &getShuntingYard(void)
            {
                return shuntingYard;
            }

            void update(float frameTime)
            {
                Plugin::Core::Log::Scope function(core->getLog(), "Population", "Update Time");

                if (frameTime == 0.0f)
                {
                    actionQueue.clear();
                }
                else
                {
                    Action action;
                    while (actionQueue.try_pop(action))
                    {
                        onAction.emit(action);
                    };
                }


                for (auto &slot : onUpdate)
                {
                    slot.second.emit(frameTime);
                }

                std::function<void(void)> entityAction;
                while (entityQueue.try_pop(entityAction))
                {
                    entityAction();
                };

                core->getLog()->setValue("Population", "Entity Count", entityMap.size());
            }

            void action(Action const &action)
            {
                actionQueue.push(action);
            }

            void load(std::string const &populationName)
            {
                loadPool.enqueue([this, populationName](void) -> void
                {
                    std::cout << "Loading population: " << populationName << std::endl;

                    const JSON::Object worldNode = JSON::Load(getContext()->getRootFileName("data", "scenes", populationName).withExtension(".json"));
                    if (!worldNode.has_member("Population"))
                    {
                        std::cerr << "Scene doesn't contain a population node: " << populationName << std::endl;
                        return;
                    }

                    auto &populationNode = worldNode.get("Population");
                    if (!populationNode.is_array())
                    {
                        std::cerr << "Scene poplation node is not an array of entities: " << populationName << std::endl;
                        return;
                    }

                    if (worldNode.has_member("Seed"))
                    {
                        shuntingYard.setRandomSeed(worldNode.get("Seed", std::mt19937::default_seed).as_uint());
                    }
                    else
                    {
                        shuntingYard.setRandomSeed(uint32_t(std::time(nullptr) & 0xFFFFFFFF));
                    }

                    JSON::Object templatesNode;
                    if (worldNode.has_member("Templates"))
                    {
                        templatesNode = worldNode.get("Templates");
                        if (!templatesNode.is_object())
                        {
                            templatesNode = JSON::EmptyObject;
                            std::cerr << "Scene templates node is not an object: " << populationName << std::endl;
                        }
                        else
                        {
                            std::cout << "Found templated block" << std::endl;
                        }
					}

					std::cout << "Found " << populationNode.size() << " Entity Definitions" << std::endl;
                    for (const auto &entityNode : populationNode.elements())
                    {
                        if (!entityNode.is_object())
                        {
                            std::cerr << "Found invalid entity node type" << std::endl;
                            continue;
                        }

                        std::vector<JSON::Member> entityComponentList;
                        if (entityNode.has_member("Template"))
                        {
                            auto &templateNode = templatesNode.get(entityNode["Template"].as_string());
                            for (const auto &componentNode : templateNode.members())
                            {
                                entityComponentList.push_back(componentNode);
                            }
                        }

                        for (const auto &componentNode : entityNode.members())
                        {
                            auto componentSearch = std::find_if(std::begin(entityComponentList), std::end(entityComponentList), [&](const JSON::Member &componentData) -> bool
                            {
                                return (componentData.name() == componentNode.name());
                            });

                            if (componentSearch == std::end(entityComponentList))
                            {
                                entityComponentList.push_back(componentNode);
                            }
                            else
                            {
                                auto &componentData = (*componentSearch);
                                for (const auto &attribute : componentNode.value().members())
                                {
                                    componentData.value().set(attribute.name(), attribute.value());
                                }
                            }
                        }

                        auto entity(std::make_unique<Entity>());
                        for (const auto &componentData : entityComponentList)
                        {
                            if (componentData.name() != "Name" &&
                                componentData.name() != "Template")
                            {
                                addComponent(entity.get(), componentData);
                            }
                        }

                        std::string entityName;
                        if (entityNode.has_member("Name"))
                        {
                            entityName = entityNode.get("Name").as_string();
                        }

                        queueEntity(entity.release(), entityName);
                    }
                });
            }

            void save(std::string const &populationName)
            {
                JSON::Array population;
                for (const auto &entityPair : entityMap)
                {
                    JSON::Object entityData;
                    entityData.set("Name", entityPair.first);

                    Entity *entity = static_cast<Entity *>(entityPair.second.get());
                    entity->listComponents([&](const std::type_index &type, const Plugin::Component::Data *data) -> void
                    {
                        auto componentName = componentNameTypeMap.find(type);
                        if (componentName == std::end(componentNameTypeMap))
                        {
                            std::cerr << "Unknown component name found when trying to save population: " << type.name() << std::endl;
                        }
                        else
                        {
                            auto component = componentMap.find(type);
                            if (component == std::end(componentMap))
                            {
                                std::cerr << "Unknown component type found when trying to save population: " << type.name() << std::endl;
                            }
                            else
                            {
                                JSON::Object componentData;
                                component->second->save(data, componentData);
                                entityData.set(componentName->second, componentData);
                            }
                        }
                    });

                    population.add(entityData);
                }

                JSON::Object scene;
                scene.set("Population", population);
                scene.set("Seed", shuntingYard.getRandomSeed());
                JSON::Save(getContext()->getRootFileName("data", "scenes", populationName).withExtension(".json"), scene);
            }

            Plugin::Entity *createEntity(std::string const &entityName, const std::vector<JSON::Member> &componentList)
            {
                auto entity(std::make_unique<Entity>());
                for (const auto &componentData : componentList)
                {
                    addComponent(entity.get(), componentData);
                }

                queueEntity(entity.release(), entityName);
                return entity.get();
            }

            void killEntity(Plugin::Entity * const entity)
            {
                entityQueue.push([this, entity](void) -> void
                {
                    auto entitySearch = std::find_if(std::begin(entityMap), std::end(entityMap), [entity](const EntityMap::value_type &entitySearch) -> bool
                    {
                        return (entitySearch.second.get() == entity);
                    });

                    if (entitySearch != std::end(entityMap))
                    {
                        onEntityDestroyed.emit(entity);
                        entityMap.erase(entitySearch);
                    }
                });
            }

            bool addComponent(Entity *entity, const JSON::Member &componentData)
            {
                assert(entity);

                auto componentNameSearch = componentTypeNameMap.find(componentData.name());
                if (componentNameSearch != std::end(componentTypeNameMap))
                {
                    auto componentSearch = componentMap.find(componentNameSearch->second);
                    if (componentSearch != std::end(componentMap))
                    {
                        Plugin::Component *componentManager = componentSearch->second.get();
                        auto component(componentManager->create());
                        componentManager->load(component.get(), componentData.value());

                        entity->addComponent(componentManager, std::move(component));
                        return true;
                    }
                    else
                    {
                        std::cerr << "Entity contains unknown component identifier: " << componentNameSearch->second.name() << std::endl;
                    }
                }
                else
                {
                    std::cerr << "Entity contains unknown component: " << componentData.name() << std::endl;
                }

                return false;
            }

            void addComponent(Plugin::Entity * const entity, const JSON::Member &componentData)
            {
                if (addComponent(static_cast<Entity *>(entity), componentData))
                {
                    onComponentAdded.emit(static_cast<Plugin::Entity *>(entity));
                }
            }

            void removeComponent(Plugin::Entity * const entity, const std::type_index &type)
            {
                assert(entity);

                if (entity->hasComponent(type))
                {
                    onComponentRemoved.emit(entity);
                    static_cast<Entity *>(entity)->removeComponent(type);
                }
            }

            void listEntities(std::function<void(Plugin::Entity *, std::string const &)> onEntity) const
            {
                concurrency::parallel_for_each(std::begin(entityMap), std::end(entityMap), [&](auto &entity) -> void
                {
                    onEntity(entity.second.get(), entity.first);
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
