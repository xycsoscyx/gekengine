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
            concurrency::concurrent_queue<Action> actionQueue;

            std::unordered_map<String, std::type_index> componentNamesMap;
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

                getContext()->listTypes(L"ComponentType", [&](const wchar_t *className) -> void
                {
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className));
                    auto componentSearch = componentMap.insert(std::make_pair(component->getIdentifier(), component));
                    if (componentSearch.second)
                    {
                        if (!componentNamesMap.insert(std::make_pair(component->getName(), component->getIdentifier())).second)
                        {
                            componentMap.erase(componentSearch.first);
                        }
                    }
                    else
                    {
                    }
                });
            }

            ~Population(void)
            {
                entityMap.clear();
                componentNamesMap.clear();
                componentMap.clear();
            }

            void loadLevel(const String &populationName)
            {
                loading = true;
                try
                {
                    actionQueue.clear();
                    entityMap.clear();
                    entityQueue.clear();
                    onLoadBegin.emit(populationName);
                    if (!populationName.empty())
                    {
                        const JSON::Object worldNode = JSON::Load(getContext()->getFileName(L"data\\scenes", populationName).append(L".json"));

                        if (worldNode.has_member(L"Seed"))
                        {
                            Evaluator::SetRandomSeed(worldNode.get(L"Seed", 0).as_uint());
                        }
                        else
                        {
                            Evaluator::SetRandomSeed(std::time(nullptr));
                        }

                        auto &templatesNode = worldNode[L"Templates"];
                        if (!templatesNode.is_object())
                        {
                            throw InvalidPrefabsBlock("Scene Templates must be an object");
                        }

                        auto &populationNode = worldNode[L"Population"];
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
                                auto componentSearch = std::find_if(entityComponentList.begin(), entityComponentList.end(), [&](const JSON::Member &componentData) -> bool
                                {
                                    return (componentData.name() == componentNode.name());
                                });

                                if (componentSearch == entityComponentList.end())
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
                                addComponent(entity.get(), componentData);
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
                catch (const std::exception &)
                {
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
                    else if (frameTime > 0.0f)
                    {
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
            }

            Plugin::Entity *createEntity(const wchar_t *entityName, const std::vector<JSON::Member> &componentList)
            {
                std::shared_ptr<Entity> entity(std::make_shared<Entity>());
                for (auto &componentData : componentList)
                {
                    addComponent(entity.get(), componentData);
                }

                onEntityCreated.emit(entity.get(), entityName);
                entityQueue.push([this, entityName = String(entityName), entity](void) -> void
                {
                    entityMap.insert(std::make_pair((entityName.empty() ? String::Format(L"unnamed_%v", ++uniqueEntityIdentifier) : entityName), entity));
                });

                return entity.get();
            }

            void killEntity(Plugin::Entity *entity)
            {
                onEntityDestroyed.emit(entity);
                entityQueue.push([this, entity](void) -> void
                {
                    auto entitySearch = std::find_if(std::begin(entityMap), std::end(entityMap), [entity](const EntityMap::value_type &entitySearch) -> bool
                    {
                        return (entitySearch.second.get() == entity);
                    });

                    if (entitySearch != std::end(entityMap))
                    {
                        entityMap.erase(entitySearch);
                    }
                });
            }

            bool addComponent(Entity *entity, const JSON::Member &componentData, std::type_index *componentIdentifier = nullptr)
            {
                GEK_REQUIRE(entity);

                auto componentNameSearch = componentNamesMap.find(componentData.name());
                if (componentNameSearch != std::end(componentNamesMap))
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
                onComponentRemoved.emit(entity, type);
                static_cast<Entity *>(entity)->removeComponent(type);
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
