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

            void removeComponent(std::type_index const &type)
            {
                auto componentSearch = componentMap.find(type);
                if (componentSearch != std::end(componentMap))
                {
                    componentMap.erase(componentSearch);
                }
            }

            void listComponents(std::function<void(std::type_index const &, Plugin::Component::Data const *)> onComponent)
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

                LockedWrite{ std::cout } << String::Format("Loading component plugins");
                getContext()->listTypes("ComponentType", [&](std::string const &className) -> void
                {
                    LockedWrite{ std::cout } << String::Format("Component found: %v", className);
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    if (componentMap.count(component->getIdentifier()) > 0)
                    {
                        LockedWrite{ std::cerr } << String::Format("Duplicate component identifier found: Class(%v), Identifier(%v)", className, component->getIdentifier().name());
                        return;
                    }

                    if (componentNameTypeMap.count(component->getIdentifier()) > 0)
                    {
                        LockedWrite{ std::cerr } << String::Format("Duplicate component name found: Class(%v), Name(%v)", className, component->getName());
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
                        LockedWrite{ std::cerr } << String::Format("Unable to add entity to scene: %v", entityName);
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
            }

            void action(Action const &action)
            {
                actionQueue.push(action);
            }

            void load(std::string const &populationName)
            {
                loadPool.enqueue([this, populationName](void) -> void
                {
                    LockedWrite{ std::cout } << String::Format("Loading population: %v", populationName);

                    JSON::Instance worldNode = JSON::Load(getContext()->getRootFileName("data", "scenes", populationName).withExtension(".json"));
                    shuntingYard.setRandomSeed(worldNode.get("Seed").convert(uint32_t(std::time(nullptr) & 0xFFFFFFFF)));

                    auto templatesNode = worldNode.get("Templates");
                    auto &populationNode = worldNode.get("Population");
                    LockedWrite{ std::cout } << String::Format("Found %v Entity Definitions", populationNode.getArray().size());
                    for (const auto &entityNode : populationNode.getArray())
                    {
                        std::vector<Component> entityComponentList;
                        if (entityNode.has_member("Template"))
                        {
                            auto &templateNode = templatesNode.get(entityNode["Template"].as_string());
                            for (const auto &componentNode : templateNode.getMembers())
                            {
                                entityComponentList.push_back(std::make_pair(componentNode.name(), componentNode.value()));
                            }
                        }

                        for (const auto &componentNode : entityNode.members())
                        {
                            auto componentSearch = std::find_if(std::begin(entityComponentList), std::end(entityComponentList), [&](Component const &componentData) -> bool
                            {
                                return (componentData.first == componentNode.name());
                            });

                            if (componentSearch == std::end(entityComponentList))
                            {
                                entityComponentList.push_back(std::make_pair(componentNode.name(), componentNode.value()));
                            }
                            else
                            {
                                auto &componentData = (*componentSearch);
                                for (const auto &attribute : componentNode.value().members())
                                {
                                    componentData.second[attribute.name()] = attribute.value();
                                }
                            }
                        }

                        auto entity(std::make_unique<Entity>());
                        for (const auto &componentData : entityComponentList)
                        {
                            if (componentData.first != "Name" &&
                                componentData.first != "Template")
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
                JSON::Object population = JSON::EmptyArray;
                for (const auto &entityPair : entityMap)
                {
                    JSON::Object entityData = JSON::EmptyObject;
                    entityData["Name"] = entityPair.first;

                    Entity *entity = static_cast<Entity *>(entityPair.second.get());
                    entity->listComponents([&](const std::type_index &type, const Plugin::Component::Data *data) -> void
                    {
                        auto componentName = componentNameTypeMap.find(type);
                        if (componentName == std::end(componentNameTypeMap))
                        {
                            LockedWrite{ std::cerr } << String::Format("Unknown component name found when trying to save population: %v", type.name());
                        }
                        else
                        {
                            auto component = componentMap.find(type);
                            if (component == std::end(componentMap))
                            {
                                LockedWrite{ std::cerr } << String::Format("Unknown component type found when trying to save population: %v", type.name());
                            }
                            else
                            {
                                JSON::Object componentData;
                                component->second->save(data, componentData);
                                entityData[componentName->second] = componentData;
                            }
                        }
                    });

                    population.add(entityData);
                }

                JSON::Object scene;
                scene["Population"] = population;
                scene["Seed"] = shuntingYard.getRandomSeed();
                JSON::Reference(scene).save(getContext()->getRootFileName("data", "scenes", populationName).withExtension(".json"));
            }

            Plugin::Entity *createEntity(std::string const &entityName, const std::vector<Component> &componentList)
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

            bool addComponent(Entity *entity, Component const &componentData)
            {
                assert(entity);

                auto componentNameSearch = componentTypeNameMap.find(componentData.first);
                if (componentNameSearch != std::end(componentTypeNameMap))
                {
                    auto componentSearch = componentMap.find(componentNameSearch->second);
                    if (componentSearch != std::end(componentMap))
                    {
                        Plugin::Component *componentManager = componentSearch->second.get();
                        auto component(componentManager->create());
                        componentManager->load(component.get(), componentData.second);

                        entity->addComponent(componentManager, std::move(component));
                        return true;
                    }
                    else
                    {
                        LockedWrite{ std::cerr } << String::Format("Entity contains unknown component identifier: %v", componentNameSearch->second.name());
                    }
                }
                else
                {
                    LockedWrite{ std::cerr } << String::Format("Entity contains unknown component: %v", componentData.first);
                }

                return false;
            }

            void addComponent(Plugin::Entity * const entity, Component const &componentData)
            {
                if (addComponent(static_cast<Entity *>(entity), componentData))
                {
                    onComponentAdded.emit(static_cast<Plugin::Entity *>(entity));
                }
            }

            void removeComponent(Plugin::Entity * const entity, std::type_index const &type)
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
