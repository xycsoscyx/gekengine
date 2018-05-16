#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/Profiler.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Population.hpp"
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

            void removeComponent(Hash type)
            {
                auto componentSearch = componentMap.find(type);
                if (componentSearch != std::end(componentMap))
                {
                    componentMap.erase(componentSearch);
                }
            }

            void listComponents(std::function<void(Hash , Plugin::Component::Data const *)> onComponent)
            {
                for (auto const &component : componentMap)
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
            bool hasComponent(Hash type) const
            {
                return (componentMap.count(type) > 0);
            }

			Plugin::Component::Data *getComponent(Hash type)
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == std::end(componentMap))
				{
                    return nullptr;
				}

				return componentSearch->second.get();
			}

			const Plugin::Component::Data *getComponent(Hash type) const
			{
				auto componentSearch = componentMap.find(type);
				if (componentSearch == std::end(componentMap))
				{
                    return nullptr;
                }

				return componentSearch->second.get();
			}
		};

        GEK_CONTEXT_USER(Population, Engine::Core *)
            , public Engine::Population
        {
        private:
            Engine::Core *core = nullptr;

            ShuntingYard shuntingYard;
            concurrency::concurrent_queue<Action> actionQueue;

            std::unordered_map<std::string, Hash> componentTypeNameMap;
            std::unordered_map<Hash, std::string> componentNameTypeMap;
            ComponentMap componentMap;

            ThreadPool<1> workerPool;
            concurrency::concurrent_queue<std::function<void(void)>> entityQueue;
            EntityList entityList;

            uint32_t uniqueEntityIdentifier = 0;

        public:
            Population(Context *context, Engine::Core *core)
                : ContextRegistration(context)
                , core(core)
            {
                assert(core);

                LockedWrite{ std::cout } << "Loading component plugins";
                getContext()->listTypes("ComponentType", [&](std::string_view className) -> void
                {
                    LockedWrite{ std::cout } << "Component found: " << className;
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className, static_cast<Plugin::Population *>(this)));
                    if (componentMap.count(component->getIdentifier()) > 0)
                    {
                        LockedWrite{ std::cerr } << "Duplicate component identifier found: Class(" << className << "), Identifier(" << component->getIdentifier() << ")";
                        return;
                    }

                    if (componentNameTypeMap.count(component->getIdentifier()) > 0)
                    {
                        LockedWrite{ std::cerr } << "Duplicate component name found: Class(" << className << "), Name(" << component->getName() << ")";
                        return;
                    }

                    componentNameTypeMap.insert(std::make_pair(component->getIdentifier(), component->getName()));
                    componentTypeNameMap.insert(std::make_pair(component->getName(), component->getIdentifier()));
                    componentMap[component->getIdentifier()] = std::move(component);
                });

                core->onShutdown.connect(this, &Population::onShutdown);
            }

            ~Population(void)
            {
                workerPool.drain();
                componentTypeNameMap.clear();
                componentMap.clear();
            }

            void queueEntity(Plugin::Entity *entity)
            {
                entityQueue.push([this, entity](void) -> void
                {
                    entityList.push_back(Plugin::EntityPtr(entity));
                    onEntityCreated(entity);
                });
            }

            // Core
            void onShutdown(void)
            {
                workerPool.reset();
                entityList.clear();
            }

            // Edit::Population
            ComponentMap &getComponentMap(void)
            {
                return componentMap;
            }

            EntityList &getEntityList(void)
            {
                return entityList;
            }

            Edit::Component *getComponent(Hash type)
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
				GEK_PROFILER_BEGIN_SCOPE(getProfiler(), 0, 0, "Population"sv, "Update"sv, Profiler::EmptyArguments)
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
							onAction(action);
						};
					}

					for (auto &slot : onUpdate)
					{
						slot.second(frameTime);
					}

					std::function<void(void)> entityAction;
					while (entityQueue.try_pop(entityAction))
					{
						entityAction();
					};
				} GEK_PROFILER_END_SCOPE();
            }

            void action(Action const &action)
            {
                actionQueue.push(action);
            }

            void reset(void)
            {
                workerPool.enqueueAndDetach([this](void) -> void
                {
                    actionQueue.clear();
                    onReset();
                    entityList.clear();
                }, __FILE__, __LINE__);
            }

            void load(std::string const &populationName)
            {
                reset();
                workerPool.enqueueAndDetach([this, populationName](void) -> void
                {
                    LockedWrite{ std::cout } << "Loading population: " << populationName;

                    JSON::Instance worldNode = JSON::Load(getContext()->findDataPath(FileSystem::CombinePaths("scenes", populationName).withExtension(".json")));
                    shuntingYard.setRandomSeed(worldNode.get("Seed").convert(uint32_t(std::time(nullptr) & 0xFFFFFFFF)));

                    auto templatesNode = worldNode.get("Templates");
                    auto &populationNode = worldNode.get("Population");
                    auto &populationArray = populationNode.getArray();
                    LockedWrite{ std::cout } << "Found " << populationArray.size() << " Entity Definitions";
                    for (auto const &entityNode : populationArray)
                    {
                        uint32_t count = 1;
                        std::vector<Component> entityComponentList;
                        if (entityNode.has_member("Template"))
                        {
                            std::string templateName;
                            auto &entityTemplateNode = entityNode["Template"];
                            if (entityTemplateNode.is_string())
                            {
                                templateName = entityTemplateNode.as_string();
                            }
                            else
                            {
                                if (entityTemplateNode.has_member("Base"))
                                {
                                    templateName = entityTemplateNode.get("Base").as_string();
                                }

                                if (entityTemplateNode.has_member("Count"))
                                {
                                    count = entityTemplateNode.get("Count").as_integer();
                                }
                            }

                            auto &templateNode = templatesNode.get(templateName);
                            for (auto const &componentNode : templateNode.getMembers())
                            {
                                entityComponentList.push_back(std::make_pair(componentNode.name(), componentNode.value()));
                            }
                        }

                        for (auto const &componentNode : entityNode.members())
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
                                for (auto const &attribute : componentNode.value().members())
                                {
                                    componentData.second[attribute.name()] = attribute.value();
                                }
                            }
                        }

                        while (count-- > 0)
                        {
                            auto populationEntity = new Entity();
                            for (auto const &componentData : entityComponentList)
                            {
                                if (componentData.first != "Template")
                                {
                                    addComponent(populationEntity, componentData);
                                }
                            }

                            auto entity = dynamic_cast<Plugin::Entity *>(populationEntity);
                            queueEntity(entity);
                        };
                    }
                }, __FILE__, __LINE__);
            }

            void save(std::string const &populationName)
            {
                JSON::Object population = JSON::EmptyArray;
                for (auto const &entity : entityList)
                {
                    JSON::Object entityData = JSON::EmptyObject;
                    Entity *editorEntity = static_cast<Entity *>(entity.get());
                    editorEntity->listComponents([&](Hash type, Plugin::Component::Data const *data) -> void
                    {
                        auto componentName = componentNameTypeMap.find(type);
                        if (componentName == std::end(componentNameTypeMap))
                        {
                            LockedWrite{ std::cerr } << "Unknown component identifier found when trying to save population: " << type;
                        }
                        else
                        {
                            auto component = componentMap.find(type);
                            if (component == std::end(componentMap))
                            {
								LockedWrite{ std::cerr } << "Unknown component type found when trying to save population: " << componentName->second << ", " << type;
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
                JSON::Reference(scene).save(getContext()->getCachePath(FileSystem::CombinePaths("scenes", populationName).withExtension(".json")));
            }

            Plugin::Entity *createEntity(std::vector<Component> const &componentList)
            {
                auto populationEntity = new Entity();
                for (auto const &componentData : componentList)
                {
                    addComponent(populationEntity, componentData);
                }

                auto entity = dynamic_cast<Plugin::Entity *>(populationEntity);
                queueEntity(entity);
                return entity;
            }

            void killEntity(Plugin::Entity * const entity)
            {
                entityQueue.push([this, entity](void) -> void
                {
                    auto entitySearch = std::find_if(std::begin(entityList), std::end(entityList), [entity](auto const &entitySearch) -> bool
                    {
                        return (entitySearch.get() == entity);
                    });

                    if (entitySearch != std::end(entityList))
                    {
                        onEntityDestroyed(entity);
                        entityList.erase(entitySearch);
                    }
                });
            }

            bool addComponent(Entity *entity, Component const &componentData)
            {
                assert(entity);

				auto componentNameSearch = std::find_if(std::begin(componentTypeNameMap), std::end(componentTypeNameMap), [&componentData](auto const &componentPair) -> bool
				{
					return (componentData.first == componentPair.first);
				});

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
                        LockedWrite{ std::cerr } << "Entity contains unknown component identifier: " << componentData.first << ", " << componentNameSearch->second;
                    }
                }
                else
                {
                    LockedWrite{ std::cerr } << "Entity contains unknown component: " << componentData.first;
                }

                return false;
            }

            void addComponent(Plugin::Entity * const entity, Component const &componentData)
            {
                if (addComponent(static_cast<Entity *>(entity), componentData))
                {
                    onComponentAdded(static_cast<Plugin::Entity *>(entity));
                }
            }

            void removeComponent(Plugin::Entity * const entity, Hash type)
            {
                assert(entity);

                if (entity->hasComponent(type))
                {
                    onComponentRemoved(entity);
                    static_cast<Entity *>(entity)->removeComponent(type);
                }
            }

            void listEntities(std::function<void(Plugin::Entity *)> onEntity) const
            {
                concurrency::parallel_for_each(std::begin(entityList), std::end(entityList), [&](auto &entity) -> void
                {
                    onEntity(entity.get());
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
