#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Context\BaseObservable.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        namespace Population
        {
            class System : public Context::BaseUser
                , public BaseObservable
                , public Population::Interface
            {
            private:
                Handle nextEntityHandle;
                std::unordered_map<CStringW, Handle> componentNameList;
                std::unordered_map<Handle, CComPtr<Component::Interface>> componentList;
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
                    INTERFACE_LIST_ENTRY_COM(Population::Interface)
                END_INTERFACE_LIST_USER

                STDMETHODIMP_(void) update(float frameTime)
                {
                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onUpdateBegin, std::placeholders::_1, frameTime)));
                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onUpdate, std::placeholders::_1, frameTime)));
                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onUpdateEnd, std::placeholders::_1, frameTime)));
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

                // Population::Interface
                STDMETHODIMP initialize(void)
                {
                    gekLogScope(__FUNCTION__);
                    gekLogMessage(L"Loading Components...");
                    HRESULT resultValue = getContext()->createEachType(__uuidof(Component::Type), [&](REFCLSID className, IUnknown *object) -> HRESULT
                    {
                        CComQIPtr<Component::Interface> component(object);
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
                                    gekLogMessage(L"ERROR: Component Name Already Used: %s", lowerCaseName.GetString());
                                }
                            }
                            else
                            {
                                gekLogMessage(L"ERROR: Component ID Already Used: 0x%08X", component->getIdentifier());
                            }
                        }

                        return S_OK;
                    });

                    return resultValue;
                }

                STDMETHODIMP_(float) getGameTime(void) const
                {
                    return 0.0f;
                }

                STDMETHODIMP load(LPCWSTR fileName)
                {
                    REQUIRE_RETURN(fileName, E_INVALIDARG);

                    gekLogScope(__FUNCTION__);
                    gekLogMessage(L"Loading Population (%s)...", fileName);

                    free();
                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onLoadBegin, std::placeholders::_1)));

                    Gek::Xml::Document xmlDocument;
                    HRESULT resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\worlds\\%s.xml", fileName));
                    if (SUCCEEDED(resultValue))
                    {
                        Gek::Xml::Node &xmlWorldNode = xmlDocument.getRoot();
                        if (xmlWorldNode && xmlWorldNode.getType().CompareNoCase(L"world") == 0)
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
                                gekLogMessage(L"ERROR: Unable to locate \"population\" node");
                                resultValue = E_UNEXPECTED;
                            }
                        }
                        else
                        {
                            gekLogMessage(L"ERROR: Unable to locate \"world\" node");
                            resultValue = E_UNEXPECTED;
                        }
                    }
                    else
                    {
                        gekLogMessage(L"ERROR: Unable to load population");
                    }

                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onLoadEnd, std::placeholders::_1, resultValue)));
                    return resultValue;
                }

                STDMETHODIMP save(LPCWSTR fileName)
                {
                    Gek::Xml::Document xmlDocument;
                    xmlDocument.create(L"world");
                    Gek::Xml::Node xmlWorldNode = xmlDocument.getRoot();
                    Gek::Xml::Node xmlPopulationNode = xmlWorldNode.createChildElement(L"population");
                    for (auto &entityHandle : entityList)
                    {
                        Gek::Xml::Node xmlEntityNode = xmlPopulationNode.createChildElement(L"entity");
                        auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const CStringW, Handle> &namedEntity) -> bool
                        {
                            return (namedEntity.second == entityHandle);
                        });

                        if (namedEntityIterator != namedEntityList.end())
                        {
                            xmlEntityNode.setAttribute(L"name", (*namedEntityIterator).first);
                        }

                        for (auto &component : componentList)
                        {
                            std::unordered_map<CStringW, CStringW> componentParameterList;
                            if (SUCCEEDED(component.second->getData(entityHandle, componentParameterList)))
                            {
                                Gek::Xml::Node xmlParameterNode = xmlEntityNode.createChildElement(component.second->getName());
                                for (auto &componentParameter : componentParameterList)
                                {
                                    if (componentParameter.first.IsEmpty())
                                    {
                                        xmlParameterNode.setText(componentParameter.second);
                                    }
                                    else
                                    {
                                        xmlParameterNode.setAttribute(componentParameter.first, componentParameter.second);
                                    }
                                }
                            }
                        }
                    }

                    xmlDocument.save(Gek::String::format(L"%%root%%\\data\\saves\\%s.xml", fileName));
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

                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onFree, std::placeholders::_1)));
                }

                STDMETHODIMP_(Handle) createEntity(const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name)
                {
                    gekLogScope(__FUNCTION__);
                    gekLogMessage(L"Creating Entity...");

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

                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onEntityCreated, std::placeholders::_1, entityHandle)));
                    return entityHandle;
                }

                STDMETHODIMP_(void) killEntity(Handle entityHandle)
                {
                    BaseObservable::sendEvent(Event<Population::Observer>(std::bind(&Population::Observer::onEntityDestroyed, std::placeholders::_1, entityHandle)));
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
    };
}; // namespace Gek
