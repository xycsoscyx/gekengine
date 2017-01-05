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
        public:
            struct Action
            {
                String name;
                ActionParameter parameter;

                Action(void)
                {
                }

                Action(const String &name, const ActionParameter &parameter)
                    : name(name)
                    , parameter(parameter)
                {
                }
            };

        private:
            Plugin::Core *core = nullptr;

            float worldTime = 0.0f;
            float frameTime = 0.0f;
            ShuntingYard shuntingYard;
            concurrency::concurrent_queue<Action> actionQueue;

            std::unordered_map<String, std::type_index> componentTypeNameMap;
            std::unordered_map<std::type_index, String> componentNameTypeMap;
            ComponentMap componentMap;

            ThreadPool loadPool;
            std::atomic<bool> loading = false;
            concurrency::concurrent_queue<String> loadQueue;
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

                core->log(L"Population", Plugin::Core::LogType::Message, L"Loading component plugins");
                getContext()->listTypes(L"ComponentType", [&](const wchar_t *className) -> void
                {
                    core->log(L"Population", Plugin::Core::LogType::Message, String::Format(L"Component found: %v", className));
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    auto componentSearch = componentMap.insert(std::make_pair(component->getIdentifier(), component));
                    if (componentSearch.second)
                    {
                        componentNameTypeMap.insert(std::make_pair(component->getIdentifier(), component->getName()));
                        auto componentNameSearch = componentTypeNameMap.insert(std::make_pair(component->getName(), component->getIdentifier()));
                        if (!componentNameSearch.second)
                        {
                            core->log(L"Population", Plugin::Core::LogType::Debug, String::Format(L"Duplicate component name found: Class(%v), Name(%v)", className, component->getName()));
                            componentMap.erase(componentSearch.first);
                        }
                    }
                    else
                    {
                        core->log(L"Population", Plugin::Core::LogType::Debug, String::Format(L"Duplicate component identifier found: Class(%v), Identifier(%v)", className, component->getIdentifier().name()));
                    }
                });
            }

            ~Population(void)
            {
                entityMap.clear();
                componentTypeNameMap.clear();
                componentMap.clear();
            }

            void loadLevel(const String &populationName)
            {
                core->log(L"Population", Plugin::Core::LogType::Message, String::Format(L"Loading population: %v", populationName));

                loading = true;
                try
                {
                    actionQueue.clear();
                    entityQueue.clear();
                    entityMap.clear();

                    onLoadBegin.emit(populationName);
                    if (!populationName.empty())
                    {
                        const JSON::Object worldNode = JSON::Load(getContext()->getRootFileName(L"data", L"scenes", populationName).append(L".json"));
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
                        }

                        auto &populationNode = worldNode.get(L"Population");
                        if (!populationNode.is_array())
                        {
                            throw InvalidPopulationBlock("Scene Population must be an array");
                        }

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

                            auto entity(std::make_shared<Entity>());
                            for (auto &componentData : entityComponentList)
                            {
                                if (componentData.name().compare(L"Name") != 0 &&
                                    componentData.name().compare(L"Template") != 0)
                                    {
                                    addComponent(entity.get(), componentData);
                                }
                            }

                            if (entityNode.has_member(L"Name"))
                            {
                                entityMap[entityNode[L"Name"].as_string()] = std::move(entity);
                            }
                            else
                            {
                                auto name(String::Format(L"unnamed_%v", ++uniqueEntityIdentifier));
                                entityMap[name] = std::move(entity);
                            }
                        }
                    }

                    frameTime = 0.0f;
                    worldTime = 0.0f;
                    onLoadSucceeded.emit(populationName);
                }
                catch (const std::exception &exception)
                {
                    core->log(L"Population", Plugin::Core::LogType::Error, String::Format(L"Unable to load population: %v", exception.what()));
                    onLoadFailed.emit(populationName);
                };

                loading = false;
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

            float getFrameTime(void) const
            {
                return frameTime;
            }

            float getWorldTime(void) const
            {
                return worldTime;
            }

            bool isLoading(void) const
            {
                return loading;
            }

            void update(float frameTime)
            {
                if (!loading)
                {
                    String populationName;
                    if (loadQueue.try_pop(populationName))
                    {
                        loadPool.enqueue([this, populationName](void) -> void
                        {
                            loadLevel(populationName);
                        });
                    }
                    else
                    {
                        Plugin::Core::Scope function(core, __FUNCTION__);

                        Action action;
                        while (actionQueue.try_pop(action))
                        {
                            onAction.emit(action.name, action.parameter);
                        };

                        this->frameTime = frameTime;
                        this->worldTime += frameTime;
                        for (auto &slot : onUpdate)
                        {
                            slot.second.emit();
                        }

                        std::function<void(void)> entityAction;
                        while (entityQueue.try_pop(entityAction))
                        {
                            entityAction();
                        };
                    }
                }
            }

            void action(const String &actionName, const ActionParameter &actionParameter)
            {
                if (!loading)
                {
                    actionQueue.push(Action(actionName, actionParameter));
                }
            }

            void load(const wchar_t *populationName)
            {
                loadQueue.push(populationName);
            }

            void save(const wchar_t *populationName)
            {
                GEK_REQUIRE(populationName);

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
                JSON::Save(getContext()->getRootFileName(L"data", L"scenes", populationName).append(L".json"), scene);
            }

            Plugin::Entity *createEntity(const wchar_t *entityName, const std::vector<JSON::Member> &componentList)
            {
                std::shared_ptr<Entity> entity(std::make_shared<Entity>());
                for (auto &componentData : componentList)
                {
                    addComponent(entity.get(), componentData);
                }

                entityQueue.push([this, entityName = String(entityName), entity = std::move(entity)](void) -> void
                {
                    if (entityMap.insert(std::make_pair((entityName.empty() ? String::Format(L"unnamed_%v", ++uniqueEntityIdentifier) : entityName), entity)).second)
                    {
                        onEntityCreated.emit(entity.get(), entityName);
                    }
                    else
                    {
                        core->log(L"Population", Plugin::Core::LogType::Error, String::Format(L"Unable to add entity to scene: %v", entityName));
                    }
                });

                return entity.get();
            }

            void killEntity(Plugin::Entity *entity)
            {
                entityQueue.push([this, entity = std::move(entity)](void) -> void
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
                        core->log(L"Population", Plugin::Core::LogType::Error, String::Format(L"Entity contains unknown component identifier: %v", componentNameSearch->second.name()));
                    }
                }
                else
                {
                    core->log(L"Population", Plugin::Core::LogType::Error, String::Format(L"Entity contains unknown component: %v", componentData.name()));
                }

                return false;
            }

            void addComponent(Plugin::Entity *entity, const JSON::Member &componentData)
            {
                std::type_index componentIdentifier(typeid(nullptr));
                if (addComponent(static_cast<Entity *>(entity), componentData, &componentIdentifier))
                {
                    onComponentAdded.emit(static_cast<Plugin::Entity *>(entity), componentIdentifier);
                }
            }

            void removeComponent(Plugin::Entity *entity, const std::type_index &type)
            {
                GEK_REQUIRE(entity);

                if (entity->hasComponent(type))
                {
                    onComponentRemoved.emit(entity, type);
                    static_cast<Entity *>(entity)->removeComponent(type);
                }
            }

            void listEntities(std::function<void(Plugin::Entity *, const wchar_t *)> onEntity) const
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
