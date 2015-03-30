﻿#include "CGEKPopulationSystem.h"
#include "GEKEngineCLSIDs.h"
#include "GEKEngine.h"
#include <ppl.h>

BEGIN_INTERFACE_LIST(CGEKPopulationSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKPopulationSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKPopulationSystem)

CGEKPopulationSystem::CGEKPopulationSystem(void)
    : m_pEngine(nullptr)
{
}

CGEKPopulationSystem::~CGEKPopulationSystem(void)
{
}

STDMETHODIMP_(void) CGEKPopulationSystem::Destroy(void)
{
    Free();
    m_aComponents.clear();
    m_aComponentSystems.clear();
}

STDMETHODIMP CGEKPopulationSystem::Initialize(IGEKEngineCore *pEngine)
{
    REQUIRE_RETURN(pEngine, E_INVALIDARG);

    m_pEngine = pEngine;
    HRESULT hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentType, [&](IUnknown *pObject) -> HRESULT
    {
        CComQIPtr<IGEKComponent> spComponent(pObject);
        if (spComponent)
        {
            m_pEngine->ShowMessage(GEKMESSAGE_NORMAL, L"population", L"Component Found : ID(0x % 08X), Name(%s)", spComponent->GetID(), spComponent->GetName());

            auto pIDIterator = m_aComponents.find(spComponent->GetID());
            auto pNamesIterator = m_aComponentNames.find(spComponent->GetName());
            if (pIDIterator != m_aComponents.end())
            {
                m_pEngine->ShowMessage(GEKMESSAGE_WARNING, L"population", L"Component ID Already Used: 0x%08X", spComponent->GetID());
            }
            else if (pNamesIterator != m_aComponentNames.end())
            {
                m_pEngine->ShowMessage(GEKMESSAGE_WARNING, L"population", L"Component Name Already Used: %s", spComponent->GetName());
            }
            else
            {
                m_aComponentNames[spComponent->GetName()] = spComponent->GetID();
                m_aComponents[spComponent->GetID()] = spComponent;
            }
        }

        return S_OK;
    });

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentSystemType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKComponentSystem> spSystem(pObject);
            if (spSystem)
            {
                if (SUCCEEDED(spSystem->Initialize(pEngine)))
                {
                    m_aComponentSystems.push_back(spSystem);
                }
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationSystem::Load(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, E_INVALIDARG);

    m_pEngine->ShowMessage(GEKMESSAGE_NORMAL, L"population", L"Loading Population (%s)...", pName);

    Free();
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadBegin, std::placeholders::_1)));

    CLibXMLDoc kDocument;
    std::list<CLibXMLNode> aEntities;
    HRESULT hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\worlds\\%s.xml", pName));
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode &kWorldNode = kDocument.GetRoot();
        if (kWorldNode.GetType().CompareNoCase(L"world") == 0)
        {
            CLibXMLNode &kPopulationNode = kWorldNode.FirstChildElement(L"population");
            if (kPopulationNode)
            {
                CLibXMLNode &kEntityNode = kPopulationNode.FirstChildElement(L"entity");
                while (kEntityNode)
                {
                    aEntities.push_back(kEntityNode);
                    kEntityNode = kEntityNode.NextSiblingElement(L"entity");
                };
            }
            else
            {
                m_pEngine->ShowMessage(GEKMESSAGE_WARNING, L"population", L"Unable to locate \"population\" node");
                hRetVal = E_UNEXPECTED;
            }
        }
        else
        {
            m_pEngine->ShowMessage(GEKMESSAGE_WARNING, L"population", L"Unable to locate \"world\" node");
            hRetVal = E_UNEXPECTED;
        }
    }
    else
    {
        m_pEngine->ShowMessage(GEKMESSAGE_WARNING, L"population", L"Unable to load population");
    }

    if (SUCCEEDED(hRetVal))
    {
        for (auto &kEntityNode : aEntities)
        {
            CStringW strName;
            std::map<CStringW, CStringW> aValues;
            kEntityNode.ListAttributes([&](LPCWSTR pName, LPCWSTR pValue) -> void
            {
                if (_wcsicmp(pName, L"name") == 0)
                {
                    strName = pValue;
                }
                else
                {
                    aValues[FormatString(L"%%%s%%", pName).MakeLower()] = FormatString(L"%f", StrToFloat(pValue));
                }
            });

            GEKENTITYID nEntityID = GEKINVALIDENTITYID;
            if (SUCCEEDED(CreateEntity(nEntityID, (strName.IsEmpty() ? nullptr : strName.GetString()))))
            {
                CLibXMLNode &kComponentNode = kEntityNode.FirstChildElement();
                while (kComponentNode)
                {
                    auto pIterator = m_aComponentNames.find(kComponentNode.GetType());
                    if (pIterator != m_aComponentNames.end())
                    {
                        std::unordered_map<CStringW, CStringW> aParams;
                        kComponentNode.ListAttributes([&aValues, &aParams](LPCWSTR pName, LPCWSTR pValue) -> void
                        {
                            CStringW strValue(pValue);
                            for (auto kPair : aValues)
                            {
                                strValue.Replace(kPair.first, kPair.second);
                            }

                            aParams[pName] = strValue;
                        });

                        AddComponent(nEntityID, (*pIterator).second, aParams);
                    }

                    kComponentNode = kComponentNode.NextSiblingElement();
                };
            }
        }
    }

    hRetVal = CGEKObservable::CheckEvent(TGEKCheck<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadEnd, std::placeholders::_1, hRetVal)));
    return hRetVal;
}

STDMETHODIMP CGEKPopulationSystem::Save(LPCWSTR pName)
{
    for (auto &nEntityID : m_aPopulation)
    {
        std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> aEntity;
        for (auto &kPair : m_aComponents)
        {
            std::unordered_map<CStringW, CStringW> aParams;
            if (SUCCEEDED(kPair.second->Serialize(nEntityID, aParams)))
            {
                aEntity[kPair.second->GetName()] = aParams;
            }
        }

        CStringW strName;
        for (auto &kPair : m_aNamedEntities)
        {
            if (kPair.second == nEntityID)
            {
                strName = kPair.first;
                break;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP_(void) CGEKPopulationSystem::Free(void)
{
    m_aHitList.clear();
    m_aPopulation.clear();
    m_aNamedEntities.clear();
    for (auto pIterator : m_aComponents)
    {
        pIterator.second->Clear();
    }

    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnFree, std::placeholders::_1)));
}

STDMETHODIMP_(void) CGEKPopulationSystem::Update(float nGameTime, float nFrameTime)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdateBegin, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdateEnd, std::placeholders::_1, nGameTime, nFrameTime)));
    for (auto nDeadID : m_aHitList)
    {
        auto pNamedIterator = std::find_if(m_aNamedEntities.begin(), m_aNamedEntities.end(), [&](std::pair<const CStringW, GEKENTITYID> &kPair) -> bool
        {
            return (kPair.second == nDeadID);
        });

        if (pNamedIterator != m_aNamedEntities.end())
        {
            m_aNamedEntities.unsafe_erase(pNamedIterator);
        }

        if (m_aPopulation.size() > 1)
        {
            auto pPopulationIterator = std::find_if(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID) -> bool
            {
                return (nEntityID == nDeadID);
            });

            if (pPopulationIterator != m_aPopulation.end())
            {
                (*pPopulationIterator) = m_aPopulation.back();
            }

            m_aPopulation.resize(m_aPopulation.size() - 1);
        }
        else
        {
            m_aPopulation.clear();
        }
    }

    m_aHitList.clear();
}

STDMETHODIMP CGEKPopulationSystem::CreateEntity(GEKENTITYID &nEntityID, LPCWSTR pName)
{
    if (pName)
    {
        auto pIterator = m_aNamedEntities.find(pName);
        if (pIterator != m_aNamedEntities.end())
        {
            return E_FAIL;
        }
    }

    static GEKENTITYID nNextEntityID = GEKINVALIDENTITYID;
    m_aPopulation.push_back(++nNextEntityID);
    nEntityID = nNextEntityID;
    if (pName)
    {
        m_aNamedEntities[pName] = nEntityID;
    }

    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityCreated, std::placeholders::_1, nEntityID)));
    return S_OK;
}

STDMETHODIMP CGEKPopulationSystem::DestroyEntity(const GEKENTITYID &nEntityID)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityDestroyed, std::placeholders::_1, nEntityID)));
    m_aHitList.push_back(nEntityID);
    return S_OK;
}

STDMETHODIMP CGEKPopulationSystem::GetNamedEntity(LPCWSTR pName, GEKENTITYID *pEntityID)
{
    REQUIRE_RETURN(pName && pEntityID, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aNamedEntities.find(pName);
    if (pIterator != m_aNamedEntities.end())
    {
        (*pEntityID) = (*pIterator).second;
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationSystem::AddComponent(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID, const std::unordered_map<CStringW, CStringW> &aParams)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(nComponentID);
    if (pIterator != m_aComponents.end())
    {
        hRetVal = (*pIterator).second->AddComponent(nEntityID);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = (*pIterator).second->DeSerialize(nEntityID, aParams);
            if (SUCCEEDED(hRetVal))
            {
                CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnComponentAdded, std::placeholders::_1, nEntityID, nComponentID)));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationSystem::RemoveComponent(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(nComponentID);
    if (pIterator != m_aComponents.end())
    {
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnComponentRemoved, std::placeholders::_1, nEntityID, nComponentID)));
        hRetVal = (*pIterator).second->RemoveComponent(nEntityID);
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKPopulationSystem::HasComponent(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    bool bReturn = false;
    auto pIterator = m_aComponents.find(nComponentID);
    if (pIterator != m_aComponents.end())
    {
        bReturn = (*pIterator).second->HasComponent(nEntityID);
    }

    return bReturn;
}

STDMETHODIMP_(LPVOID) CGEKPopulationSystem::GetComponent(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
{
    auto pIterator = m_aComponents.find(nComponentID);
    if (pIterator != m_aComponents.end())
    {
        return (*pIterator).second->GetComponent(nEntityID);
    }

    return nullptr;
}

STDMETHODIMP_(void) CGEKPopulationSystem::ListEntities(std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel)
{
    if (bParallel)
    {
        concurrency::parallel_for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID) -> void
        {
            OnEntity(nEntityID);
        });
    }
    else
    {
        for (auto &nEntityID : m_aPopulation)
        {
            OnEntity(nEntityID);
        }
    }
}

STDMETHODIMP_(void) CGEKPopulationSystem::ListComponentsEntities(const std::vector<GEKCOMPONENTID> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel)
{
    std::list<IGEKComponent *> aComponentList;
    for (auto &nComponentID : aComponents)
    {
        auto pIterator = m_aComponents.find(nComponentID);
        if (pIterator != m_aComponents.end())
        {
            aComponentList.push_back((*pIterator).second);
        }
    }

    if (bParallel)
    {
        concurrency::parallel_for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID) -> void
        {
            bool bEntityHasAllComponents = true;
            for (auto &spComponent : aComponentList)
            {
                if (!spComponent->HasComponent(nEntityID))
                {
                    bEntityHasAllComponents = false;
                }
            }

            if (bEntityHasAllComponents)
            {
                OnEntity(nEntityID);
            }
        });
    }
    else
    {
        for (auto &nEntityID : m_aPopulation)
        {
            bool bEntityHasAllComponents = true;
            for (auto &spComponent : aComponentList)
            {
                if (!spComponent->HasComponent(nEntityID))
                {
                    bEntityHasAllComponents = false;
                }
            }

            if (bEntityHasAllComponents)
            {
                OnEntity(nEntityID);
            }
        }
    }
}
