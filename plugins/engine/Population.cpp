#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Context\ObservableMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

#include "GEK\Components\Transform.h"

namespace Gek
{
    namespace Engine
    {
        namespace Population
        {
            class EntityClass : public UnknownMixin
                , public Entity
            {
            private:
                std::unordered_map<std::type_index, std::pair<Component::Interface *, LPVOID>> componentList;

            public:
                EntityClass(void)
                {
                }

                ~EntityClass(void)
                {
                    for (auto componentPair : componentList)
                    {
                        componentPair.second.first->destroy(componentPair.second.second);
                    }
                }

                BEGIN_INTERFACE_LIST(EntityClass)
                    INTERFACE_LIST_ENTRY_COM(Entity)
                END_INTERFACE_LIST_UNKNOWN

                void addComponent(Component::Interface *component, LPVOID data)
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

            class System : public ContextUserMixin
                , public ObservableMixin
                , public Engine::Population::Interface
            {
            private:
                std::unordered_map<CStringW, std::type_index> componentNameList;
                std::unordered_map<std::type_index, CComPtr<Component::Interface>> componentList;

                concurrency::concurrent_vector<CAdapt<CComPtr<Entity>>> entityList;
                concurrency::concurrent_unordered_map<CStringW, Entity *> namedEntityList;
                concurrency::concurrent_vector<Entity *> killEntityList;

            public:
                System(void)
                {
                }

                ~System(void)
                {
                    free();
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Observable::Interface)
                    INTERFACE_LIST_ENTRY_COM(Engine::Population::Interface)
                END_INTERFACE_LIST_USER

                // Engine::Population::Interface
                STDMETHODIMP initialize(IUnknown *initializerContext)
                {
                    gekLogScope();
                    gekLogMessage(L"Loading Components...");

                    REQUIRE_RETURN(initializerContext, E_INVALIDARG);

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
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onUpdateBegin, std::placeholders::_1, frameTime)));
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onUpdate, std::placeholders::_1, frameTime)));
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onUpdateEnd, std::placeholders::_1, frameTime)));
                    for (auto const killEntity : killEntityList)
                    {
                        auto namedEntityIterator = std::find_if(namedEntityList.begin(), namedEntityList.end(), [&](std::pair<const CStringW, Entity *> &namedEntity) -> bool
                        {
                            return (namedEntity.second == killEntity);
                        });

                        if (namedEntityIterator != namedEntityList.end())
                        {
                            namedEntityList.unsafe_erase(namedEntityIterator);
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

                STDMETHODIMP load(LPCWSTR fileName)
                {
                    gekLogScope(fileName);

                    REQUIRE_RETURN(fileName, E_INVALIDARG);

                    free();
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onLoadBegin, std::placeholders::_1)));

                    Gek::Xml::Document xmlDocument;
                    HRESULT resultValue = xmlDocument.load(Gek::String::format(L"%%root%%\\data\\worlds\\%s.xml", fileName));
                    if (SUCCEEDED(resultValue))
                    {
                        Gek::Xml::Node xmlWorldNode = xmlDocument.getRoot();
                        if (xmlWorldNode && xmlWorldNode.getType().CompareNoCase(L"world") == 0)
                        {
                            Gek::Xml::Node xmlPopulationNode = xmlWorldNode.firstChildElement(L"population");
                            if (xmlPopulationNode)
                            {
                                Gek::Xml::Node xmlEntityNode = xmlPopulationNode.firstChildElement(L"entity");
                                while (xmlEntityNode)
                                {
                                    std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> entityParameterList;
                                    Gek::Xml::Node xmlComponentNode = xmlEntityNode.firstChildElement();
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

                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onLoadEnd, std::placeholders::_1, resultValue)));
                    return resultValue;
                }

                STDMETHODIMP save(LPCWSTR fileName)
                {
                    Gek::Xml::Document xmlDocument;
                    xmlDocument.create(L"world");
                    Gek::Xml::Node xmlWorldNode = xmlDocument.getRoot();
                    Gek::Xml::Node xmlPopulationNode = xmlWorldNode.createChildElement(L"population");

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
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onFree, std::placeholders::_1)));
                }

                STDMETHODIMP_(Entity *) createEntity(const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &entityParameterList, LPCWSTR name)
                {
                    gekLogScope();

                    CComPtr<EntityClass> entity = new EntityClass();
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
                                    Component::Interface *component = componentPair->second;
                                    LPVOID componentData = component->create(componentParameterList);
                                    if (componentData)
                                    {
                                        entity->addComponent(component, componentData);
                                    }
                                }
                            }
                        }

                        ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onEntityCreated, std::placeholders::_1, entity)));
                    }

                    return entity;
                }

                STDMETHODIMP_(void) killEntity(Entity *entity)
                {
                    ObservableMixin::sendEvent(Event<Engine::Population::Observer>(std::bind(&Engine::Population::Observer::onEntityDestroyed, std::placeholders::_1, entity)));
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
            };

            REGISTER_CLASS(System)
        }; // namespace Population
    }; // namespace Engine
}; // namespace Gek
