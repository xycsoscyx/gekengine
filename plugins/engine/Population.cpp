#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include <map>
#include <ppl.h>

namespace Gek
{
    class EntityImplementation
        : public Entity
    {
    private:
        std::unordered_map<std::type_index, std::pair<Component *, LPVOID>> componentList;

    public:
        EntityImplementation(void)
        {
        }

        ~EntityImplementation(void)
        {
            for (auto &componentPair : componentList)
            {
                componentPair.second.first->destroyData(componentPair.second.second);
            }
        }

        void addComponent(Component *component, LPVOID data)
        {
            componentList[component->getIdentifier()] = std::make_pair(component, data);
        }

        void removeComponent(const std::type_index &type)
        {
            auto componentPair = componentList.find(type);
            if (componentPair != componentList.end())
            {
                componentPair->second.first->destroyData(componentPair->second.second);
                componentList.erase(componentPair);
            }
        }

        // Entity
        bool hasComponent(const std::type_index &type)
        {
            return (componentList.count(type) > 0);
        }

        LPVOID getComponent(const std::type_index &type)
        {
            return componentList[type].second;
        }
    };

    class PopulationImplementation
        : public ContextRegistration<PopulationImplementation, EngineContext *>
        , virtual public ObservableMixin
        , public PopulationSystem
    {
    private:
        EngineContext *engine;

        float worldTime;
        float frameTime;

        std::unordered_map<wstring, std::type_index> componentNameList;
        std::unordered_map<std::type_index, ComponentPtr> componentList;
        std::list<ProcessorPtr> processorList;

        std::vector<EntityPtr> entityList;
        std::unordered_map<wstring, Entity *> namedEntityList;
        std::vector<Entity *> killEntityList;

        typedef std::multimap<UINT32, std::pair<UINT32, PopulationObserver *>> UpdatePriorityMap;
        UpdatePriorityMap updatePriorityMap;

        std::map<UINT32, UpdatePriorityMap::value_type *> updateHandleMap;

    public:
        PopulationImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , engine(engine)
        {
        }

        ~PopulationImplementation(void)
        {
            processorList.clear();
            componentNameList.clear();
            componentList.clear();
        }

        // PopulationSystem
        void loadPlugins(void)
        {
            getContext()->listTypes(L"ComponentType", [&](const wchar_t *className) -> void
            {
                ComponentPtr component(getContext()->createClass<Component>(className));
                auto identifierIterator = componentList.insert(std::make_pair(component->getIdentifier(), component));
                if (identifierIterator.second)
                {
                    if (!componentNameList.insert(std::make_pair(component->getName(), component->getIdentifier())).second)
                    {
                        componentList.erase(identifierIterator.first);
                    }
                }
                else
                {
                }
            });

            getContext()->listTypes(L"ProcessorType", [&](const wchar_t *className) -> void
            {
                processorList.push_back(getContext()->createClass<Processor>(className, engine));
            });
        }

        // Population
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
                auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const wstring, Entity *> &namedEntity) -> bool
                {
                    return (namedEntity.second == killEntity);
                });

                if (namedEntityIterator != namedEntityList.end())
                {
                    namedEntityList.erase(namedEntityIterator);
                }

                if (entityList.size() > 1)
                {
                    auto entityIterator = std::find_if(entityList.begin(), entityList.end(), [killEntity](const EntityPtr &entity) -> bool
                    {
                        return (entity.get() == killEntity);
                    });

                    if (entityIterator != entityList.end())
                    {
                        (*entityIterator) = entityList.back();
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
        void load(const wstring &fileName)
        {
            loadScene = [this, fileName](void) -> void
            {
                try
                {
                    free();
                    sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadBegin, std::placeholders::_1)));

                    Gek::XmlDocumentPtr document(XmlDocument::load(Gek::String::format(L"$root\\data\\scenes\\%v.xml", fileName)));
                    Gek::XmlNodePtr worldNode = document->getRoot(L"world");
                    Gek::XmlNodePtr populationNode = worldNode->firstChildElement(L"population");
                    Gek::XmlNodePtr entityNode = populationNode->firstChildElement(L"entity");
                    while (entityNode->isValid())
                    {
                        EntityDefinition entityDefinition;
                        Gek::XmlNodePtr componentNode = entityNode->firstChildElement();
                        while (componentNode)
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

                            componentNode = componentNode->nextSiblingElement();
                        };

                        if (entityNode->hasAttribute(L"name"))
                        {
                            createEntity(entityDefinition, entityNode->getAttribute(L"name"));
                        }
                        else
                        {
                            createEntity(entityDefinition, nullptr);
                        }

                        entityNode = entityNode->nextSiblingElement(L"entity");
                    };

                    frameTime = 0.0f;
                    worldTime = 0.0f;
                    sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadSucceeded, std::placeholders::_1)));
                }
                catch (BaseException exception)
                {
                    sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadFailed, std::placeholders::_1)));
                };
            };
        }

        void save(const wstring &fileName)
        {
            Gek::XmlDocumentPtr document(XmlDocument::create(L"world"));
            Gek::XmlNodePtr worldNode = document->getRoot(L"world");
            Gek::XmlNodePtr populationNode = worldNode->createChildElement(L"population");
            for (auto &entity : entityList)
            {
            }

            document->save(Gek::String::format(L"$root\\data\\saves\\%v.xml", fileName));
        }

        void free(void)
        {
            sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onFree, std::placeholders::_1)));
            namedEntityList.clear();
            killEntityList.clear();
            entityList.clear();
        }

        Entity * createEntity(const EntityDefinition &entityData, const wchar_t *name)
        {
            std::shared_ptr<EntityImplementation> entity = std::make_shared<EntityImplementation>();
            for (auto &componentDataPair : entityData)
            {
                auto &componentName = componentDataPair.first;
                auto &componentData = componentDataPair.second;
                auto componentIterator = componentNameList.find(componentName);
                if (componentIterator != componentNameList.end())
                {
                    std::type_index componentIdentifier = componentIterator->second;
                    auto componentPair = componentList.find(componentIdentifier);
                    if (componentPair != componentList.end())
                    {
                        Component *componentManager = componentPair->second.get();
                        LPVOID component = componentManager->createData(componentData);
                        if (component)
                        {
                            entity->addComponent(componentManager, component);
                        }
                    }
                }
            }

            EntityPtr baseEntity(std::dynamic_pointer_cast<Entity>(entity));

            entityList.push_back(baseEntity);
            sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityCreated, std::placeholders::_1, baseEntity.get())));
            if (name)
            {
                namedEntityList[name] = baseEntity.get();
            }

            return baseEntity.get();
        }

        void killEntity(Entity *entity)
        {
            sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityDestroyed, std::placeholders::_1, entity)));
            killEntityList.push_back(entity);
        }

        Entity * getNamedEntity(const wchar_t *name) const
        {
            GEK_REQUIRE(name);

            Entity* entity = nullptr;
            auto namedEntity = namedEntityList.find(name);
            if (namedEntity != namedEntityList.end())
            {
                return (*namedEntity).second;
            }

            return nullptr;
        }

        void listEntities(std::function<void(Entity *)> onEntity) const
        {
            concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](const EntityPtr &entity) -> void
            {
                onEntity(entity.get());
            });
        }

        void listProcessors(std::function<void(Processor *)> onProcessor) const
        {
            concurrency::parallel_for_each(processorList.begin(), processorList.end(), [&](const ProcessorPtr &processor) -> void
            {
                onProcessor(processor.get());
            });
        }

        UINT32 setUpdatePriority(PopulationObserver *observer, UINT32 priority)
        {
            static UINT32 nextHandle = 0;
            UINT32 updateHandle = InterlockedIncrement(&nextHandle);
            auto pair = std::make_pair(priority, std::make_pair(updateHandle, observer));
            auto updateIterator = updatePriorityMap.insert(pair);
            updateHandleMap[updateHandle] = &(*updateIterator);
            return updateHandle;
        }

        void removeUpdatePriority(UINT32 updateHandle)
        {
            auto handleIterator = updateHandleMap.find(updateHandle);
            if (handleIterator != updateHandleMap.end())
            {
                UINT32 priority = handleIterator->second->first;
                auto priorityRange = updatePriorityMap.equal_range(priority);
                auto priorityIterator = std::find_if(priorityRange.first, priorityRange.second, [&](auto &priorityPair) -> bool
                {
                    return (handleIterator->second == &priorityPair);
                });

                if (priorityIterator != updatePriorityMap.end())
                {
                    updatePriorityMap.erase(priorityIterator);
                }

                updateHandleMap.erase(handleIterator);
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(PopulationImplementation);
}; // namespace Gek
