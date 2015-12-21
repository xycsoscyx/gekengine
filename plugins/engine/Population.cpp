#include "GEK\Engine\Population.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\Component.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <map>
#include <ppl.h>

#include "GEK\Components\Transform.h"

namespace Gek
{
    class EntityImplementation : public UnknownMixin
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
            for (auto componentPair : componentList)
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

    class PopulationImplementation : public ContextUserMixin
        , public ObservableMixin
        , public Population
    {
    private:
        std::unordered_map<CStringW, std::type_index> componentNameList;
        std::unordered_map<std::type_index, CComPtr<Component>> componentList;

        std::vector<CAdapt<CComPtr<Entity>>> entityList;
        std::unordered_map<CStringW, Entity *> namedEntityList;
        std::vector<Entity *> killEntityList;

        std::multimap<UINT32, PopulationObserver *> updatePriorityMap;
        std::map<UINT32, std::pair<UINT32, PopulationObserver *>> updateHandleMap;

    public:
        PopulationImplementation(void)
        {
        }

        ~PopulationImplementation(void)
        {
            free();
        }

        BEGIN_INTERFACE_LIST(PopulationImplementation)
            INTERFACE_LIST_ENTRY_COM(Observable)
            INTERFACE_LIST_ENTRY_COM(Population)
        END_INTERFACE_LIST_USER

        // Population
        STDMETHODIMP initialize(IUnknown *initializerContext)
        {
            gekLogScope();
            gekLogMessage(L"Loading Components...");

            REQUIRE_RETURN(initializerContext, E_INVALIDARG);

            HRESULT resultValue = getContext()->createEachType(__uuidof(ComponentType), [&](REFCLSID className, IUnknown *object) -> HRESULT
            {
                CComQIPtr<Component> component(object);
                if (component)
                {
                    CStringW lowerCaseName(component->getName());
                    lowerCaseName.MakeLower();

                    auto identifierIterator = componentList.insert(std::make_pair(component->getIdentifier(), component));
                    if (identifierIterator.second)
                    {
                        gekLogMessage(L"Component Found : ID(0x % 08X), Name(%s)", component->getIdentifier(), lowerCaseName.GetString());
                        if (!componentNameList.insert(std::make_pair(lowerCaseName, component->getIdentifier())).second)
                        {
                            componentList.erase(identifierIterator.first);
                            gekLogMessage(L"[error] Component Name Already Used: %s", lowerCaseName.GetString());
                        }
                    }
                    else
                    {
                        gekLogMessage(L"[error] Component ID Already Used: 0x%08X", component->getIdentifier());
                    }
                }

                return S_OK;
            });

            return resultValue;
        }

        STDMETHODIMP_(void) update(float frameTime)
        {
            if (doLoad)
            {
                doLoad();
                doLoad = nullptr;
            }
            else
            {
                for (auto priorityPair : updatePriorityMap)
                {
                    priorityPair.second->onUpdate(frameTime);
                }

                for (auto const killEntity : killEntityList)
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
        }

        std::function<void(void)> doLoad;
        STDMETHODIMP load(LPCWSTR fileName)
        {
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekLogScope(fileName);

            doLoad = std::bind([&](const CStringW &fileName) -> void
            {
                free();
                ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadBegin, std::placeholders::_1)));

                Gek::XmlDocument xmlDocument;
                HRESULT resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\worlds\\%s.xml", fileName));
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
                                std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> entityParameterList;
                                Gek::XmlNode xmlComponentNode = xmlEntityNode.firstChildElement();
                                while (xmlComponentNode)
                                {
                                    std::unordered_map<CStringW, CStringW> &componentParameterList = entityParameterList[xmlComponentNode.getType()];
                                    xmlComponentNode.listAttributes([&componentParameterList](LPCWSTR name, LPCWSTR value) -> void
                                    {
                                        componentParameterList.insert(std::make_pair(name, value));
                                    });

                                    componentParameterList[L""] = xmlComponentNode.getText();
                                    xmlComponentNode = xmlComponentNode.nextSiblingElement();
                                };

                                if (xmlEntityNode.hasAttribute(L"name"))
                                {
                                    createEntity(entityParameterList, xmlEntityNode.getAttribute(L"name"));
                                }
                                else
                                {
                                    createEntity(entityParameterList, nullptr);
                                }

                                xmlEntityNode = xmlEntityNode.nextSiblingElement(L"entity");
                            };
                        }
                        else
                        {
                            gekLogMessage(L"[error] Unable to locate \"population\" node");
                            resultValue = E_UNEXPECTED;
                        }
                    }
                    else
                    {
                        gekLogMessage(L"[error] Unable to locate \"world\" node");
                        resultValue = E_UNEXPECTED;
                    }
                }
                else
                {
                    gekLogMessage(L"[error] Unable to load population");
                }

                ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onLoadEnd, std::placeholders::_1, resultValue)));
            }, CStringW(fileName));

            return S_OK;
        }

        STDMETHODIMP save(LPCWSTR fileName)
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
            killEntityList.clear();
            entityList.clear();
            namedEntityList.clear();
            ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onFree, std::placeholders::_1)));
        }

        STDMETHODIMP_(Entity *) createEntity(const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name)
        {
            gekLogScope();

            CComPtr<EntityImplementation> entity = new EntityImplementation();
            if (entity)
            {
                entityList.push_back(CComPtr<Entity>(entity->getClass<Entity>()));
                for (auto componentParameterPair : entityParameterList)
                {
                    auto &componentName = componentParameterPair.first;
                    auto &componentParameterList = componentParameterPair.second;
                    auto componentIdentifierPair = componentNameList.find(componentName);
                    if (componentIdentifierPair == componentNameList.end())
                    {
                        gekLogMessage(L"Unable to find component name: %s", componentName.GetString());
                    }
                    else
                    {
                        std::type_index componentIdentifier = componentIdentifierPair->second;
                        auto componentPair = componentList.find(componentIdentifier);
                        if (componentPair == componentList.end())
                        {
                            gekLogMessage(L"Unable to find component identifier: 0x%08X", componentIdentifier.hash_code());
                        }
                        else
                        {
                            Component *component = componentPair->second;
                            LPVOID componentData = component->create(componentParameterList);
                            if (componentData)
                            {
                                entity->addComponent(component, componentData);
                            }
                        }
                    }
                }

                ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityCreated, std::placeholders::_1, entity)));
            }

            if (name)
            {
                namedEntityList[name] = entity;
            }

            return entity;
        }

        STDMETHODIMP_(void) killEntity(Entity *entity)
        {
            ObservableMixin::sendEvent(Event<PopulationObserver>(std::bind(&PopulationObserver::onEntityDestroyed, std::placeholders::_1, entity)));
            killEntityList.push_back(entity);
        }

        STDMETHODIMP_(Entity *) getNamedEntity(LPCWSTR name)
        {
            REQUIRE_RETURN(name, nullptr);

            Entity* entity = nullptr;
            auto namedEntity = namedEntityList.find(name);
            if (namedEntity != namedEntityList.end())
            {
                entity = (*namedEntity).second;
            }

            return entity;
        }

        STDMETHODIMP_(void) listEntities(std::function<void(Entity *)> onEntity, bool runInParallel)
        {
            if (runInParallel)
            {
                concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](const CComPtr<Entity> &entity) -> void
                {
                    onEntity(entity);
                });
            }
            else
            {
                std::for_each(entityList.begin(), entityList.end(), [&](const CComPtr<Entity> &entity) -> void
                {
                    onEntity(entity);
                });
            }
        }

        STDMETHODIMP_(UINT32) setUpdatePriority(PopulationObserver *observer, UINT32 priority)
        {
            auto pair = std::make_pair(priority, observer);
            auto updateIterator = updatePriorityMap.insert(pair);

            static UINT32 nextValue = 0;
            UINT32 returnValue = InterlockedIncrement(&nextValue);
            updateHandleMap[returnValue] = pair;

            return returnValue;
        }

        STDMETHODIMP_(void) removeUpdatePriority(UINT32 updateHandle)
        {
            auto handleIterator = updateHandleMap.find(updateHandle);
            if (handleIterator != updateHandleMap.end())
            {
                UINT32 priority = handleIterator->second.first;
                PopulationObserver *observer = handleIterator->second.second;
                auto priorityIterator = updatePriorityMap.equal_range(priority);
                auto updateIterator = priorityIterator.first;
                while (updateIterator != priorityIterator.second)
                {
                    auto currentIterator = updateIterator++;
                    PopulationObserver *currentObserver = currentIterator->second;
                    if (currentObserver == observer)
                    {
                        updatePriorityMap.erase(currentIterator);
                    }
                };
            }
        }
    };

    REGISTER_CLASS(PopulationImplementation)
}; // namespace Gek
