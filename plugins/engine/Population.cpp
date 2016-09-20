﻿#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include <future>
#include <map>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        class Entity
            : public Plugin::Entity
        {
        private:
            std::unordered_map<std::type_index, std::pair<Plugin::Component *, void *>> componentsMap;

        public:
            ~Entity(void)
            {
                for (auto &componentInfo : componentsMap)
                {
                    componentInfo.second.first->destroy(componentInfo.second.second);
                }
            }

            void addComponent(Plugin::Component *component, void *data)
            {
                componentsMap[component->getIdentifier()] = std::make_pair(component, data);
            }

            void removeComponent(const std::type_index &type)
            {
                auto componentSearch = componentsMap.find(type);
                if (componentSearch != componentsMap.end())
                {
                    componentSearch->second.first->destroy(componentSearch->second.second);
                    componentsMap.erase(componentSearch);
                }
            }

            // Plugin::Entity
            bool hasComponent(const std::type_index &type) const
            {
                return (componentsMap.count(type) > 0);
            }

			void *getComponent(const std::type_index &type)
			{
				auto componentSearch = componentsMap.find(type);
				if (componentSearch == componentsMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.second;
			}

			const void *getComponent(const std::type_index &type) const
			{
				auto componentSearch = componentsMap.find(type);
				if (componentSearch == componentsMap.end())
				{
					throw ComponentNotFound();
				}

				return componentSearch->second.second;
			}
		};

        GEK_CONTEXT_USER(Population, Plugin::Core *)
            , public Plugin::Population
        {
        private:
            Plugin::Core *core;

            float worldTime;
            float frameTime;

            std::unordered_map<String, std::type_index> componentNamesMap;
            std::unordered_map<std::type_index, Plugin::ComponentPtr> componentsMap;

            std::mutex loadMutex;
            std::future<void> loadThread;

            std::vector<Plugin::EntityPtr> entityList;
            std::unordered_map<String, Plugin::Entity *> namedEntityMap;
            std::vector<Plugin::Entity *> killEntityList;

        public:
            Population(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
            {
                getContext()->listTypes(L"ComponentType", [&](const wchar_t *className) -> void
                {
                    Plugin::ComponentPtr component(getContext()->createClass<Plugin::Component>(className));
                    auto componentSearch = componentsMap.insert(std::make_pair(component->getIdentifier(), component));
                    if (componentSearch.second)
                    {
                        if (!componentNamesMap.insert(std::make_pair(component->getName(), component->getIdentifier())).second)
                        {
                            componentsMap.erase(componentSearch.first);
                        }
                    }
                    else
                    {
                    }
                });
            }

            ~Population(void)
            {
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    loadThread.wait();
                }

                namedEntityMap.clear();
                killEntityList.clear();
                entityList.clear();
                componentNamesMap.clear();
                componentsMap.clear();
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

            void update(bool isBackgroundProcess, float frameTime)
            {
                Plugin::PopulationStep::State state = Plugin::PopulationStep::State::Unknown;
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    state = Plugin::PopulationStep::State::Loading;
                }
                else if(isBackgroundProcess)
                {
                    state = Plugin::PopulationStep::State::Idle;
                }
                else
                {
                    state = Plugin::PopulationStep::State::Active;
                    this->frameTime = frameTime;
                    this->worldTime += frameTime;
                }
                
                Sequencer::sendUpdate(&Plugin::PopulationStep::onUpdate, state);
                for (auto const &killEntity : killEntityList)
                {
                    auto namedSearch = std::find_if(namedEntityMap.begin(), namedEntityMap.end(), [&](auto &namedEntityPair) -> bool
                    {
                        return (namedEntityPair.second == killEntity);
                    });

                    if (namedSearch != namedEntityMap.end())
                    {
                        namedEntityMap.erase(namedSearch);
                    }

                    if (entityList.size() > 1)
                    {
                        auto entitySearch = std::find_if(entityList.begin(), entityList.end(), [killEntity](auto &entity) -> bool
                        {
                            return (entity.get() == killEntity);
                        });

                        if (entitySearch != entityList.end())
                        {
                            (*entitySearch) = entityList.back();
                        }

                        entityList.resize(entityList.size() - 1);
                    }
                    else
                    {
                        entityList.clear();
                    }
                }

                killEntityList.clear();
            }

            void load(const wchar_t *populationName)
            {
                if (loadThread.valid() && loadThread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    loadThread.wait();
                }

                std::lock_guard<std::mutex> lock(loadMutex);
                loadThread = std::async(std::launch::async, [this, populationName = String(populationName)](void) -> void
                {
                    try
                    {
                        namedEntityMap.clear();
                        killEntityList.clear();
                        entityList.clear();

                        sendShout(&Plugin::PopulationListener::onLoadBegin);

                        Xml::Node worldNode = Xml::load(getContext()->getFileName(L"data\\scenes", populationName).append(L".xml"), L"world");

                        std::unordered_map<String, EntityDefinition> prefabsMap;
                        for (auto &prefabNode : worldNode.getChild(L"prefabs").children)
                        {
                            EntityDefinition &entityDefinition = prefabsMap[prefabNode.type];
                            for(auto &componentNode : prefabNode.children)
                            {
                                auto &componentData = entityDefinition[componentNode.type];
                                componentData.insert(componentNode.attributes.begin(), componentNode.attributes.end());
                                componentData.value = componentNode.text;
                            }
                        }

                        for(auto &entityNode : worldNode.getChild(L"population").children)
                        {
                            EntityDefinition entityDefinition;
                            auto prefabSearch = prefabsMap.find(entityNode.getAttribute(L"prefab"));
                            if (prefabSearch != prefabsMap.end())
                            {
                                entityDefinition = prefabSearch->second;
                            }
                            
                            for (auto &componentNode : entityNode.children)
                            {
                                auto &componentData = entityDefinition[componentNode.type];
                                for (auto &attribute : componentNode.attributes)
                                {
                                    componentData[attribute.first] = attribute.second;
                                }

                                if (!componentNode.text.empty())
                                {
                                    componentData.value = componentNode.text;
                                }
                            }

                            if (entityNode.attributes.count(L"name"))
                            {
                                createEntity(entityDefinition, entityNode.getAttribute(L"name"));
                            }
                            else
                            {
                                createEntity(entityDefinition, nullptr);
                            }
                        }

                        frameTime = 0.0f;
                        worldTime = 0.0f;
                        sendShout(&Plugin::PopulationListener::onLoadSucceeded);
                    }
                    catch (const std::exception &)
                    {
                        sendShout(&Plugin::PopulationListener::onLoadFailed);
                    };
                });
            }

            void save(const wchar_t *populationName)
            {
            }

            Plugin::Entity * addEntity(Plugin::EntityPtr entity, const wchar_t *entityName)
            {
                entityList.push_back(entity);
                sendShout(&Plugin::PopulationListener::onEntityCreated, entity.get());
                if (entityName)
                {
                    namedEntityMap[entityName] = entity.get();
                }

                return entity.get();
            }

            Plugin::Entity * createEntity(const EntityDefinition &entityDefinition, const wchar_t *entityName)
            {
                std::shared_ptr<Entity> entity(std::make_shared<Entity>());
                for (auto &componentInfo : entityDefinition)
                {
                    auto &componentName = componentInfo.first;
                    auto &componentData = componentInfo.second;
                    auto componentNameSearch = componentNamesMap.find(componentName);
                    if (componentNameSearch != componentNamesMap.end())
                    {
                        std::type_index componentIdentifier(componentNameSearch->second);
                        auto componentSearch = componentsMap.find(componentIdentifier);
                        if (componentSearch != componentsMap.end())
                        {
                            Plugin::Component *componentManager = componentSearch->second.get();
                            LPVOID component = componentManager->create(componentData);
                            if (component)
                            {
                                entity->addComponent(componentManager, component);
                            }
                        }
                    }
                }

                return addEntity(entity, entityName);
            }

            void killEntity(Plugin::Entity *entity)
            {
                sendShout(&Plugin::PopulationListener::onEntityDestroyed, entity);
                killEntityList.push_back(entity);
            }

            Plugin::Entity * getNamedEntity(const wchar_t *entityName) const
            {
                auto namedSearch = namedEntityMap.find(entityName);
                if (namedSearch != namedEntityMap.end())
                {
                    return namedSearch->second;
                }

                return nullptr;
            }

            void listEntities(std::function<void(Plugin::Entity *)> onEntity) const
            {
                concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](auto &entity) -> void
                {
                    onEntity(entity.get());
                });
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
