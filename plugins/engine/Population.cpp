#include "GEK\Engine\Population.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Context\Plugin.h"
#include "GEK\Context\ObservableMixin.h"
#include <map>
#include <ppl.h>

#include "GEK\Components\Transform.h"

namespace Gek
{
    class EntityImplementation
        : public UnknownMixin
        , public Entity
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
                componentPair.second.first->destroy(componentPair.second.second);
            }
        }

        BEGIN_INTERFACE_LIST(EntityImplementation)
            INTERFACE_LIST_ENTRY_COM(Entity)
        END_INTERFACE_LIST_UNKNOWN

        void addComponent(Component *component, LPVOID data)
        {
            componentList[component->getIdentifier()] = std::make_pair(component, data);
        }

        void removeComponent(const std::type_index &type)
        {
            auto componentPair = componentList.find(type);
            if (componentPair != componentList.end())
            {
                componentPair->second.first->destroy(componentPair->second.second);
                componentList.erase(componentPair);
            }
        }

        // Entity
        STDMETHODIMP_(bool) hasComponent(const std::type_index &type)
        {
            return (componentList.count(type) > 0);
        }

        STDMETHODIMP_(LPVOID) getComponent(const std::type_index &type)
        {
            return componentList[type].second;
        }
    };

    class PopulationImplementation
        : public ContextUserMixin
        , public ObservableMixin
        , public Population
    {
    private:
        float worldTime;
        float frameTime;

        std::unordered_map<CStringW, std::type_index> componentNameList;
        std::unordered_map<std::type_index, CComPtr<Component>> componentList;
        std::list<CAdapt<CComPtr<Processor>>> processorList;

        std::vector<CAdapt<CComPtr<Entity>>> entityList;
        std::unordered_map<CStringW, Entity *> namedEntityList;
        std::vector<Entity *> killEntityList;

        typedef std::multimap<UINT32, std::pair<UINT32, PopulationObserver *>> UpdatePriorityMap;
        UpdatePriorityMap updatePriorityMap;

        std::map<UINT32, UpdatePriorityMap::value_type *> updateHandleMap;

    public:
        PopulationImplementation(void)
        {
        }

        ~PopulationImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PopulationImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(Population)
        END_INTERFACE_LIST_USER

        // Population
        STDMETHODIMP_(void) destroy(void)
        {
            processorList.clear();
            componentNameList.clear();
            componentList.clear();
        }

        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            GEK_REQUIRE(initializerContext);

            HRESULT resultValue = E_FAIL;
            resultValue = getContext()->createEachType(__uuidof(ComponentType), [&](REFCLSID className, IUnknown *object) -> HRESULT
            {
                CComQIPtr<Component> component(object);
                if (component)
                {
                    CStringW lowerCaseName(component->getName());
                    lowerCaseName.MakeLower();

                    auto identifierIterator = componentList.insert(std::make_pair(component->getIdentifier(), component));
                    if (identifierIterator.second)
                    {
                        if (!componentNameList.insert(std::make_pair(lowerCaseName, component->getIdentifier())).second)
                        {
                            componentList.erase(identifierIterator.first);
                        }
                    }
                    else
                    {
                    }
                }

                return S_OK;
            });

            if (SUCCEEDED(resultValue))
            {
                resultValue = getContext()->createEachType(__uuidof(ProcessorType), [&](REFCLSID className, IUnknown *object) -> HRESULT
                {
                    HRESULT resultValue = E_FAIL;
                    CComQIPtr<Processor> system(object);
                    if (system)
                    {
                        resultValue = system->initialize(initializerContext);
                        if (SUCCEEDED(resultValue))
                        {
                            processorList.push_back(system);
                        }
                    }

                    return S_OK;
                });
            }

            return resultValue;
        }

        STDMETHODIMP_(float) getFrameTime(void)
        {
            return frameTime;
        }

        STDMETHODIMP_(float) getWorldTime(void)
        {
            return worldTime;
        }

        STDMETHODIMP_(void) update(bool isIdle, float frameTime)
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
                auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const CStringW, Entity *> &namedEntity) -> bool
                {
                    return (namedEntity.second == killEntity);
                });

                if (namedEntityIterator != namedEntityList.end())
                {
                    namedEntityList.erase(namedEntityIterator);
                }

                if (entityList.size() > 1)
                {
                    auto entityIterator = std::find(entityList.begin(), entityList.end(), killEntity);
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
        STDMETHODIMP load(const wchar_t *fileName)
        {
            loadScene = std::bind([&](const CStringW &fileName) -> HRESULT
            {
                HRESULT resultValue = E_FAIL;

                free();
                ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadBegin, std::placeholders::_1)));

                Gek::XmlDocument xmlDocument;
                resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\scenes\\%s.xml", fileName));
                if (SUCCEEDED(resultValue))
                {
                    Gek::XmlNode xmlWorldNode = xmlDocument.getRoot();
                    if (xmlWorldNode && xmlWorldNode.getType().CompareNoCase(L"world") == 0)
                    {
                        Gek::XmlNode xmlPopulationNode = xmlWorldNode.firstChildElement(L"population");
                        if (xmlPopulationNode)
                        {
                            Gek::XmlNode xmlEntityNode = xmlPopulationNode.firstChildElement(L"entity");
                            while (xmlEntityNode)
                            {
                                EntityDefinition entityDefinition;
                                Gek::XmlNode xmlComponentNode = xmlEntityNode.firstChildElement();
                                while (xmlComponentNode)
                                {
                                    auto &componentData = entityDefinition[xmlComponentNode.getType()];
                                    xmlComponentNode.listAttributes([&componentData](const wchar_t *name, const wchar_t *value) -> void
                                    {
                                        componentData.insert(std::make_pair(name, value));
                                    });

                                    if (!xmlComponentNode.getText().IsEmpty())
                                    {
                                        componentData.SetString(xmlComponentNode.getText());
                                    }

                                    xmlComponentNode = xmlComponentNode.nextSiblingElement();
                                };

                                if (xmlEntityNode.hasAttribute(L"name"))
                                {
                                    createEntity(entityDefinition, xmlEntityNode.getAttribute(L"name"));
                                }
                                else
                                {
                                    createEntity(entityDefinition, nullptr);
                                }

                                xmlEntityNode = xmlEntityNode.nextSiblingElement(L"entity");
                            };
                        }
                        else
                        {
                            resultValue = E_UNEXPECTED;
                        }
                    }
                    else
                    {
                        resultValue = E_UNEXPECTED;
                    }
                }
                else
                {
                }

                frameTime = 0.0f;
                worldTime = 0.0f;
                ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadEnd, std::placeholders::_1, resultValue)));
                return resultValue;
            }, CStringW(fileName));

            return S_OK;
        }

        STDMETHODIMP save(const wchar_t *fileName)
        {
            Gek::XmlDocument xmlDocument;
            xmlDocument.create(L"world");
            Gek::XmlNode xmlWorldNode = xmlDocument.getRoot();
            Gek::XmlNode xmlPopulationNode = xmlWorldNode.createChildElement(L"population");

            for (auto &entity : entityList)
            {
            }

            xmlDocument.save(Gek::String::format(L"%%root%%\\data\\saves\\%s.xml", fileName));
            return S_OK;
        }

        STDMETHODIMP_(void) free(void)
        {
            ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onFree, std::placeholders::_1)));
            namedEntityList.clear();
            killEntityList.clear();
            entityList.clear();
        }

        STDMETHODIMP_(Entity *) createEntity(const EntityDefinition &entityData, const wchar_t *name)
        {
            CComPtr<EntityImplementation> entity = new EntityImplementation();
            if (entity)
            {
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
                            Component *componentManager = componentPair->second;
                            LPVOID component = componentManager->create(componentData);
                            if (component)
                            {
                                entity->addComponent(componentManager, component);
                            }
                        }
                    }
                }

                if (entity)
                {
                    entityList.push_back(CComPtr<Entity>(entity->getClass<Entity>()));
                    ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityCreated, std::placeholders::_1, entity)));
                }
            }

            if (name && entity)
            {
                namedEntityList[name] = entity;
            }

            return entity.p;
        }

        STDMETHODIMP_(void) killEntity(Entity *entity)
        {
            ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityDestroyed, std::placeholders::_1, entity)));
            killEntityList.push_back(entity);
        }

        STDMETHODIMP_(Entity *) getNamedEntity(const wchar_t *name)
        {
            GEK_REQUIRE(name);

            Entity* entity = nullptr;
            auto namedEntity = namedEntityList.find(name);
            if (namedEntity != namedEntityList.end())
            {
                entity = (*namedEntity).second;
            }

            return entity;
        }

        STDMETHODIMP_(void) listEntities(std::function<void(Entity *)> onEntity)
        {
            concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](const CComPtr<Entity> &entity) -> void
            {
                onEntity(entity);
            });
        }

        STDMETHODIMP_(void) listProcessors(std::function<void(Processor *)> onProcessor)
        {
            concurrency::parallel_for_each(processorList.begin(), processorList.end(), [&](const CComPtr<Processor> &processor) -> void
            {
                onProcessor(processor.p);
            });
        }

        STDMETHODIMP_(UINT32) setUpdatePriority(PopulationObserver *observer, UINT32 priority)
        {
            static UINT32 nextHandle = 0;
            UINT32 updateHandle = InterlockedIncrement(&nextHandle);
            auto pair = std::make_pair(priority, std::make_pair(updateHandle, observer));
            auto updateIterator = updatePriorityMap.insert(pair);
            updateHandleMap[updateHandle] = &(*updateIterator);
            return updateHandle;
        }

        STDMETHODIMP_(void) removeUpdatePriority(UINT32 updateHandle)
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

    REGISTER_CLASS(PopulationImplementation)
}; // namespace Gek
