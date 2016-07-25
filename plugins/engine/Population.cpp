#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
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
            bool hasComponent(const std::type_index &type)
            {
                return (componentsMap.count(type) > 0);
            }

            void *getComponent(const std::type_index &type)
            {
                auto componentSearch = componentsMap.find(type);
                GEK_CHECK_CONDITION(componentSearch == componentsMap.end(), Exception, "Plugin::Entity doesn't contain component %v", type.name());
                return (*componentSearch).second.second;
            }
        };

        GEK_CONTEXT_USER(Population, Plugin::Core *)
            , public Engine::Population
        {
        private:
            Plugin::Core *core;

            float worldTime;
            float frameTime;

            std::unordered_map<String, std::type_index> componentNamesMap;
            std::unordered_map<std::type_index, Plugin::ComponentPtr> componentsMap;
            std::list<Plugin::ProcessorPtr> processorList;

            std::vector<Plugin::EntityPtr> entityList;
            std::unordered_map<String, Plugin::Entity *> namedEntityMap;
            std::vector<Plugin::Entity *> killEntityList;

            using UpdatePriorityMap = std::multimap<uint32_t, std::pair<uint32_t, Plugin::PopulationListener *>>;
            UpdatePriorityMap updatePriorityMap;

            std::map<uint32_t, UpdatePriorityMap::value_type *> updateHandleMap;

        public:
            Population(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
            {
                OutputDebugString(L"");
            }

            // Engine::Population
            void loadComponents(void)
            {
                GEK_TRACE_SCOPE();
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

                getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
                {
                    processorList.push_back(getContext()->createClass<Plugin::Processor>(className, core));
                });
            }

            void freeComponents(void)
            {
                processorList.clear();
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

            void update(bool isIdle, float frameTime)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(isIdle), GEK_PARAMETER(frameTime));
                if (!isIdle)
                {
                    this->frameTime = frameTime;
                    this->worldTime += frameTime;
                }

                if (loadScene)
                {
                    loadScene();
                    loadScene = nullptr;
                }

                for (auto &priorityPair : updatePriorityMap)
                {
                    priorityPair.second.second->onUpdate(priorityPair.second.first, isIdle);
                }

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

            std::function<void(void)> loadScene;
            void load(const wchar_t *populationName)
            {
                loadScene = [this, populationName = String(populationName)](void) -> void
                {
                    GEK_TRACE_SCOPE(GEK_PARAMETER(populationName));
                    try
                    {
                        free();
                        sendEvent(&Plugin::PopulationListener::onLoadBegin);

                        XmlDocumentPtr document(XmlDocument::load(String(L"$root\\data\\scenes\\%v.xml", populationName)));
                        XmlNodePtr worldNode(document->getRoot(L"world"));

                        std::unordered_map<String, EntityDefinition> prefabsMap;
                        XmlNodePtr prefabsNode(worldNode->firstChildElement(L"prefabs"));
                        for (XmlNodePtr prefabNode(prefabsNode->firstChildElement()); prefabNode->isValid(); prefabNode = prefabNode->nextSiblingElement())
                        {
                            EntityDefinition &entityDefinition = prefabsMap[prefabNode->getType()];
                            for (XmlNodePtr componentNode(prefabNode->firstChildElement()); componentNode->isValid(); componentNode = componentNode->nextSiblingElement())
                            {
                                auto &componentData = entityDefinition[componentNode->getType()];
                                componentNode->listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                                {
                                    componentData.unordered_map::insert(std::make_pair(name, value));
                                });

                                if (!componentNode->getText().empty())
                                {
                                    componentData.value = componentNode->getText();
                                }
                            }
                        }

                        XmlNodePtr populationNode(worldNode->firstChildElement(L"population"));
                        for (XmlNodePtr entityNode(populationNode->firstChildElement(L"entity")); entityNode->isValid(); entityNode = entityNode->nextSiblingElement(L"entity"))
                        {
                            EntityDefinition entityDefinition;
                            auto prefabSearch = prefabsMap.find(entityNode->getAttribute(L"prefab"));
                            if (prefabSearch != prefabsMap.end())
                            {
                                entityDefinition = (*prefabSearch).second;
                            }

                            for (XmlNodePtr componentNode(entityNode->firstChildElement()); componentNode->isValid(); componentNode = componentNode->nextSiblingElement())
                            {
                                auto &componentData = entityDefinition[componentNode->getType()];
                                componentNode->listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                                {
                                    componentData[name] = value;
                                });

                                if (!componentNode->getText().empty())
                                {
                                    componentData.value = componentNode->getText();
                                }
                            }

                            if (entityNode->hasAttribute(L"name"))
                            {
                                createEntity(entityDefinition, entityNode->getAttribute(L"name"));
                            }
                            else
                            {
                                createEntity(entityDefinition, nullptr);
                            }
                        }

                        frameTime = 0.0f;
                        worldTime = 0.0f;
                        sendEvent(&Plugin::PopulationListener::onLoadSucceeded);
                    }
                    catch (const Gek::Exception &)
                    {
                        sendEvent(&Plugin::PopulationListener::onLoadFailed);
                    };
                };
            }

            void save(const wchar_t *populationName)
            {
                GEK_TRACE_SCOPE(GEK_PARAMETER(populationName));
                GEK_REQUIRE(populationName);

                XmlDocumentPtr document(XmlDocument::create(L"world"));
                XmlNodePtr worldNode(document->getRoot(L"world"));
                XmlNodePtr populationNode(worldNode->createChildElement(L"population"));
                for (auto &entity : entityList)
                {
                }

                document->save(String(L"$root\\data\\saves\\%v.xml", populationName));
            }

            void free(void)
            {
                sendEvent(&Plugin::PopulationListener::onFree);
                namedEntityMap.clear();
                killEntityList.clear();
                entityList.clear();
            }

            Plugin::Entity * addEntity(Plugin::EntityPtr entity, const wchar_t *entityName)
            {
                entityList.push_back(entity);
                sendEvent(&Plugin::PopulationListener::onEntityCreated, entity.get());
                if (entityName)
                {
                    namedEntityMap[entityName] = entity.get();
                }

                return entity.get();
            }

            Plugin::Entity * createEntity(const EntityDefinition &entityDefinition, const wchar_t *entityName)
            {
                std::shared_ptr<Entity> entity(makeShared<Entity>());
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
                sendEvent(&Plugin::PopulationListener::onEntityDestroyed, entity);
                killEntityList.push_back(entity);
            }

            Plugin::Entity * getNamedEntity(const wchar_t *entityName) const
            {
                GEK_REQUIRE(entityName);

                Plugin::Entity* entity = nullptr;
                auto namedSearch = namedEntityMap.find(entityName);
                if (namedSearch != namedEntityMap.end())
                {
                    return (*namedSearch).second;
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

            void listProcessors(std::function<void(Plugin::Processor *)> onProcessor) const
            {
                concurrency::parallel_for_each(processorList.begin(), processorList.end(), [&](auto &processor) -> void
                {
                    onProcessor(processor.get());
                });
            }

            uint32_t setUpdatePriority(Plugin::PopulationListener *observer, uint32_t priority)
            {
                static uint32_t nextHandle = 0;
                uint32_t updateHandle = InterlockedIncrement(&nextHandle);
                auto pair = std::make_pair(priority, std::make_pair(updateHandle, observer));
                auto updateSearch = updatePriorityMap.insert(pair);
                updateHandleMap[updateHandle] = &(*updateSearch);
                return updateHandle;
            }

            void removeUpdatePriority(uint32_t updateHandle)
            {
                auto updateSearch = updateHandleMap.find(updateHandle);
                if (updateSearch != updateHandleMap.end())
                {
                    uint32_t priority = updateSearch->second->first;
                    auto priorityRange = updatePriorityMap.equal_range(priority);
                    auto prioritySearch = std::find_if(priorityRange.first, priorityRange.second, [&](auto &priorityPair) -> bool
                    {
                        return (updateSearch->second == &priorityPair);
                    });

                    if (prioritySearch != updatePriorityMap.end())
                    {
                        updatePriorityMap.erase(prioritySearch);
                    }

                    updateHandleMap.erase(updateSearch);
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Population);
    }; // namespace Implementation
}; // namespace Gek
