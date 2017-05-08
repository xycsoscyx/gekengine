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
					throw ComponentNotFound("Entity does not contain requested component");
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(const std::type_index &type) const
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == std::end(componentMap))
				{
                    throw ComponentNotFound("Entity does not contain requested component");
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

            std::unordered_map<WString, std::type_index> componentTypeNameMap;
            std::unordered_map<std::type_index, WString> componentNameTypeMap;
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
                GEK_REQUIRE(core);

                core->getLog()->message("Population", Plugin::Core::Log::Type::Message, "Loading component plugins");
                getContext()->listTypes(L"ComponentType", [&](WString const &className) -> void
                {
                    core->getLog()->message("Population", Plugin::Core::Log::Type::Message, "Component found: %v", className);
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    if (componentMap.count(component->getIdentifier()) > 0)
                    {
                        core->getLog()->message("Population", Plugin::Core::Log::Type::Debug, "Duplicate component identifier found: Class(%v), Identifier(%v)", className, component->getIdentifier().name());
                        return;
                    }

                    if (componentNameTypeMap.count(component->getIdentifier()) > 0)
                    {
                        core->getLog()->message("Population", Plugin::Core::Log::Type::Debug, "Duplicate component name found: Class(%v), Name(%v)", className, component->getName());
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
                core->onExit.disconnect<Population, &Population::onExit>(this);

                entityMap.clear();
                componentTypeNameMap.clear();
                componentMap.clear();
            }

            void queueEntity(Plugin::Entity *entity, WString const & requestedName)
            {
                entityQueue.push([this, entity = entity, requestedName = requestedName](void) -> void
                {
                    auto entityName(requestedName.empty() ? WString::Format(L"unnamed_%v", ++uniqueEntityIdentifier) : requestedName);
                    if (entityMap.count(requestedName) > 0)
                    {
                        core->getLog()->message("Population", Plugin::Core::Log::Type::Error, "Unable to add entity to scene: %v", entityName);
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
                loadPool.clear();
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

            void load(WString const &populationName)
            {
                loadPool.enqueue([this, populationName](void) -> void
                {
                    core->getLog()->message("Population", Plugin::Core::Log::Type::Message, "Loading population: %v", populationName);

                    try
                    {
                        const JSON::Object worldNode = JSON::Load(getContext()->getRootFileName(L"data", L"scenes", populationName).withExtension(L".json"));
                        if (!worldNode.has_member(L"Population"))
                        {
                            throw InvalidPopulationBlock("Scene must contain a population list");
                        }

                        if (worldNode.has_member(L"Seed"))
                        {
                            shuntingYard.setRandomSeed(worldNode.get(L"Seed", std::mt19937::default_seed).as_uint());
                        }
                        else
                        {
                            shuntingYard.setRandomSeed(uint32_t(std::time(nullptr) & 0xFFFFFFFF));
                        }

                        JSON::Object templatesNode;
                        if (worldNode.has_member(L"Templates"))
                        {
                            templatesNode = worldNode.get(L"Templates");
                            if (!templatesNode.is_object())
                            {
                                throw InvalidTemplatesBlock("Scene Templates must be an object");
                            }

							core->getLog()->message("Population", Plugin::Core::Log::Type::Message, "Found templated block");
						}

                        auto &populationNode = worldNode.get(L"Population");
                        if (!populationNode.is_array())
                        {
                            throw InvalidPopulationBlock("Scene Population must be an array");
                        }

						core->getLog()->message("Population", Plugin::Core::Log::Type::Message, "Found %v Entity Definitions", populationNode.size());
                        for (auto &entityNode : populationNode.elements())
                        {
                            if (!entityNode.is_object())
                            {
                                throw InvalidEntityBlock("Scene entity must be an object");
                            }

                            std::vector<JSON::Member> entityComponentList;
                            if (entityNode.has_member(L"Template"))
                            {
                                auto &templateNode = templatesNode.get(entityNode[L"Template"].as_string());
                                for (auto &componentNode : templateNode.members())
                                {
                                    entityComponentList.push_back(componentNode);
                                }
                            }

                            for (auto &componentNode : entityNode.members())
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
                                    for (auto &attribute : componentNode.value().members())
                                    {
                                        componentData.value().set(attribute.name(), attribute.value());
                                    }
                                }
                            }

                            auto entity(std::make_unique<Entity>());
                            for (auto &componentData : entityComponentList)
                            {
                                if (componentData.name().compare(L"Name") != 0 &&
                                    componentData.name().compare(L"Template") != 0)
                                {
                                    std::type_index componentIdentifier(typeid(nullptr));
                                    addComponent(entity.get(), componentData, &componentIdentifier);
                                }
                            }

                            WString entityName;
                            if (entityNode.has_member(L"Name"))
                            {
                                entityName = entityNode.get(L"Name").as_string();
                            }

                            queueEntity(entity.release(), entityName);
                        }
                    }
                    catch (const std::exception &exception)
                    {
                        core->getLog()->message("Population", Plugin::Core::Log::Type::Error, "Unable to load population: %v", exception.what());
                    };
                });
            }

            void save(WString const &populationName)
            {
                JSON::Array population;
                for (auto &entityPair : entityMap)
                {
                    JSON::Object entityData;
                    entityData.set(L"Name", entityPair.first);

                    Entity *entity = static_cast<Entity *>(entityPair.second.get());
                    entity->listComponents([&](const std::type_index &type, const Plugin::Component::Data *data) -> void
                    {
                        auto componentName = componentNameTypeMap.find(type);
                        if (componentName == std::end(componentNameTypeMap))
                        {
                            throw FatalError("Unknown component name found when trying to save population");
                        }

                        auto component = componentMap.find(type);
                        if (component == std::end(componentMap))
                        {
                            throw FatalError("Unknown component type found when trying to save population");
                        }

                        JSON::Object componentData;
                        component->second->save(data, componentData);
                        entityData.set(componentName->second, componentData);
                    });

                    population.add(entityData);
                }

                JSON::Object scene;
                scene.set(L"Population", population);
                scene.set(L"Seed", shuntingYard.getRandomSeed());
                JSON::Save(getContext()->getRootFileName(L"data", L"scenes", populationName).withExtension(L".json"), scene);
            }

            Plugin::Entity *createEntity(WString const &entityName, const std::vector<JSON::Member> &componentList)
            {
                auto entity(std::make_unique<Entity>());
                for (auto &componentData : componentList)
                {
                    std::type_index componentIdentifier(typeid(nullptr));
                    addComponent(entity.get(), componentData, &componentIdentifier);
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

            bool addComponent(Entity *entity, const JSON::Member &componentData, std::type_index *componentIdentifier = nullptr)
            {
                GEK_REQUIRE(entity);

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
                        if (componentIdentifier)
                        {
                            (*componentIdentifier) = componentNameSearch->second;
                        }

                        return true;
                    }
                    else
                    {
                        core->getLog()->message("Population", Plugin::Core::Log::Type::Error, "Entity contains unknown component identifier: %v", componentNameSearch->second.name());
                    }
                }
                else
                {
                    core->getLog()->message("Population", Plugin::Core::Log::Type::Error, "Entity contains unknown component: %v", componentData.name());
                }

                return false;
            }

            void addComponent(Plugin::Entity * const entity, const JSON::Member &componentData)
            {
                std::type_index componentIdentifier(typeid(nullptr));
                if (addComponent(static_cast<Entity *>(entity), componentData, &componentIdentifier))
                {
                    onComponentAdded.emit(static_cast<Plugin::Entity *>(entity), componentIdentifier);
                }
            }

            void removeComponent(Plugin::Entity * const entity, const std::type_index &type)
            {
                GEK_REQUIRE(entity);

                if (entity->hasComponent(type))
                {
                    onComponentRemoved.emit(entity, type);
                    static_cast<Entity *>(entity)->removeComponent(type);
                }
            }

            void listEntities(std::function<void(Plugin::Entity *, WString const &)> onEntity) const
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
