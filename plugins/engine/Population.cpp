#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Context\Observable.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

DECLARE_INTERFACE(ComponentInterface);

namespace Gek
{
    namespace Population
    {
        class System : public ContextUser
                     , public Observable
                     , public Population::SystemInterface
        {
        private:
            Handle nextEntityHandle;
            std::unordered_map<CStringW, Handle> componentNameList;
            std::unordered_map<Handle, CComPtr<ComponentInterface>> componentList;
            concurrency::concurrent_vector<Handle> entityList;
            concurrency::concurrent_unordered_map<CStringW, Handle> namedEntityList;
            concurrency::concurrent_vector<Handle> killEntityList;

        public:
            System(void)
                : nextEntityHandle(InvalidHandle)
            {
            }

            ~System(void)
            {
                free();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(Gek::ObservableInterface)
                INTERFACE_LIST_ENTRY_COM(Population::SystemInterface)
            END_INTERFACE_LIST_UNKNOWN
/*
            STDMETHODIMP Initialize(IGEKEngineCore *pEngine)
            {
                REQUIRE_RETURN(pEngine, E_INVALIDARG);

                m_pEngine = pEngine;
                HRESULT resultValue = GetContext()->CreateEachType(CLSID_GEKComponentType, [&](REFCLSID kCLSID, IUnknown *pObject) -> HRESULT
                {
                    CComQIPtr<IGEKComponent> scomponent(pObject);
                    if (scomponent)
                    {
                        m_pEngine->ShowMessage(GEKMESSAGE_NORMAL, L"population", L"Component Found : ID(0x % 08X), Name(%s)", scomponent->GetID(), scomponent->GetName());
                        if (!componentNameList.insert(std::make_pair(scomponent->GetName(), scomponent->GetID())).second)
                        {
                            gekLogMessage(L"Component Name Already Used: %s", scomponent->GetName());
                        }

                        if (!componentList.insert(std::make_pair(scomponent->GetID(), scomponent)).second)
                        {
                            gekLogMessage(L"Component ID Already Used: 0x%08X", scomponent->GetID());
                        }
                    }

                    return S_OK;
                });

                return resultValue;
            }
*/
            STDMETHODIMP_(void) update(float frameTime)
            {
                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onUpdateBegin, std::placeholders::_1, frameTime)));
                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onUpdate, std::placeholders::_1, frameTime)));
                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onUpdateEnd, std::placeholders::_1, frameTime)));
                for (auto killEntityHandle : killEntityList)
                {
                    auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const CStringW, Handle> &namedEntity) -> bool
                    {
                        return (namedEntity.second == killEntityHandle);
                    });

                    if (namedEntityIterator != namedEntityList.end())
                    {
                        namedEntityList.unsafe_erase(namedEntityIterator);
                    }

                    if (entityList.size() > 1)
                    {
                        for (auto component : componentList)
                        {
                            component.second->removeComponent(killEntityHandle);
                        }

                        auto entityIterator = std::find_if(entityList.begin(), entityList.end(), [&](Handle entityHandle) -> bool
                        {
                            return (entityHandle == killEntityHandle);
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

            // Population::SystemInterface
            STDMETHODIMP_(float) getGameTime(void) const
            {
                return 0.0f;
            }

            STDMETHODIMP load(LPCWSTR fileName)
            {
                REQUIRE_RETURN(fileName, E_INVALIDARG);

                gekLogMessage(L"Loading Population (%s)...", fileName);

                free();
                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onLoadBegin, std::placeholders::_1)));

                Gek::Xml::Document xmlDocument;
                HRESULT resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\worlds\\%s.xml", fileName));
                if (SUCCEEDED(resultValue))
                {
                    Gek::Xml::Node &xmlWorldNode = xmlDocument.getRoot();
                    if (xmlWorldNode.getType().CompareNoCase(L"world") == 0)
                    {
                        Gek::Xml::Node &xmlPopulationNode = xmlWorldNode.firstChildElement(L"population");
                        if (xmlPopulationNode)
                        {
                            Gek::Xml::Node &xmlEntityNode = xmlPopulationNode.firstChildElement(L"entity");
                            while (xmlEntityNode)
                            {
                                std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> entityParameterList;
                                Gek::Xml::Node &xmlComponentNode = xmlEntityNode.firstChildElement();
                                while (xmlComponentNode)
                                {
                                    std::unordered_map<CStringW, CStringW> &componentParameterList = entityParameterList[xmlComponentNode.getType()];
                                    xmlComponentNode.listAttributes([&componentParameterList](LPCWSTR name, LPCWSTR value) -> void
                                    {
                                        componentParameterList.insert(std::make_pair(name, value));
                                    });

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
                            gekLogMessage(L"Unable to locate \"population\" node");
                            resultValue = E_UNEXPECTED;
                        }
                    }
                    else
                    {
                        gekLogMessage(L"Unable to locate \"world\" node");
                        resultValue = E_UNEXPECTED;
                    }
                }
                else
                {
                    gekLogMessage(L"Unable to load population");
                }

                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onLoadEnd, std::placeholders::_1, resultValue)));
                return resultValue;
            }

            STDMETHODIMP save(LPCWSTR fileName)
            {
                for (auto &entityHandle : entityList)
                {
                    auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const CStringW, Handle> &namedEntity) -> bool
                    {
                        return (namedEntity.second == entityHandle);
                    });

                    std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> entityParameterList;
                    for (auto &component : componentList)
                    {
                        std::unordered_map<CStringW, CStringW> componentParameterList;
                        if (SUCCEEDED(component.second->getData(entityHandle, componentParameterList)))
                        {
                            entityParameterList.insert(std::make_pair(component.second->getName(), componentParameterList));
                        }
                    }
                }

                return S_OK;
            }

            STDMETHODIMP_(void) free(void)
            {
                killEntityList.clear();
                entityList.clear();
                namedEntityList.clear();
                for (auto component : componentList)
                {
                    component.second->clear();
                }

                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onFree, std::placeholders::_1)));
            }

            STDMETHODIMP_(Handle) createEntity(const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name)
            {
                Handle entityHandle = InterlockedIncrement(&nextEntityHandle);
                if (name != nullptr)
                {
                    if (!namedEntityList.insert(std::make_pair(name, entityHandle)).second)
                    {
                        return InvalidHandle;
                    }
                }

                entityList.push_back(entityHandle);
                for (auto componentParameterList : entityParameterList)
                {
                    auto component = componentNameList.find(componentParameterList.first);
                    if (component == componentNameList.end())
                    {
                        gekLogMessage(L"Unable to find component name: %s", componentParameterList.first.GetString());
                    }
                    else
                    {
                        auto pIterator = componentList.find(component->second);
                        if (pIterator == componentList.end())
                        {
                            gekLogMessage(L"Unable to find component ID: 0x%08X", component->second);
                        }
                        else
                        {
                            (*pIterator).second->addComponent(entityHandle);
                            (*pIterator).second->setData(entityHandle, componentParameterList.second);
                        }
                    }
                }

                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onEntityCreated, std::placeholders::_1, entityHandle)));
                return entityHandle;
            }

            STDMETHODIMP_(void) killEntity(Handle entityHandle)
            {
                Observable::sendEvent(Event<Population::ObserverInterface>(std::bind(&Population::ObserverInterface::onEntityDestroyed, std::placeholders::_1, entityHandle)));
                killEntityList.push_back(entityHandle);
            }

            STDMETHODIMP_(Handle) getNamedEntity(LPCWSTR name)
            {
                REQUIRE_RETURN(name, InvalidHandle);

                Handle entityHandle = InvalidHandle;
                auto pIterator = namedEntityList.find(name);
                if (pIterator != namedEntityList.end())
                {
                    entityHandle = (*pIterator).second;
                }

                return entityHandle;
            }

            STDMETHODIMP_(void) listEntities(std::function<void(Handle)> onEntity, bool runInParallel)
            {
                if (runInParallel)
                {
                    concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](Handle entityHandle) -> void
                    {
                        onEntity(entityHandle);
                    });
                }
                else
                {
                    for (auto &entityHandle : entityList)
                    {
                        onEntity(entityHandle);
                    }
                }
            }

            STDMETHODIMP_(void) listEntities(const std::vector<Handle> &requiredComponentList, std::function<void(Handle)> onEntity, bool runInParallel)
            {
                std::set<Handle> entityList;
                for (auto &requiredComponent : requiredComponentList)
                {
                    auto pIterator = componentList.find(requiredComponent);
                    if (pIterator != componentList.end())
                    {
                        (*pIterator).second->getIntersectingSet(entityList);
                        if (entityList.empty())
                        {
                            break;
                        }
                    }
                }

                if (!entityList.empty())
                {
                    if (runInParallel)
                    {
                        concurrency::parallel_for_each(entityList.begin(), entityList.end(), [&](Handle entityHandle) -> void
                        {
                            onEntity(entityHandle);
                        });
                    }
                    else
                    {
                        for (auto &entityHandle : entityList)
                        {
                            onEntity(entityHandle);
                        }
                    }
                }
            }

            STDMETHODIMP_(bool) hasComponent(Handle entityHandle, Handle componentHandle)
            {
                bool bReturn = false;
                auto pIterator = componentList.find(componentHandle);
                if (pIterator != componentList.end())
                {
                    bReturn = (*pIterator).second->hasComponent(entityHandle);
                }

                return bReturn;
            }

            STDMETHODIMP_(LPVOID) getComponent(Handle entityHandle, Handle componentHandle)
            {
                auto pIterator = componentList.find(componentHandle);
                if (pIterator != componentList.end())
                {
                    return (*pIterator).second->getComponent(entityHandle);
                }

                return nullptr;
            }
        };

        REGISTER_CLASS(System)
    }; // namespace Population
}; // namespace Gek
