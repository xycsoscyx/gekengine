#include "CGEKPopulationManager.h"
#include <algorithm>
#include <thread>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKPopulationManager)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKPopulationManager)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKPopulationManager)

CGEKPopulationManager::CGEKPopulationManager(void)
{
}

CGEKPopulationManager::~CGEKPopulationManager(void)
{
}

STDMETHODIMP CGEKPopulationManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKPopulationManager, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKComponent> spComponent(pObject);
            if (spComponent)
            {
                auto pIterator = m_aComponents.find(spComponent->GetName());
                if (pIterator == m_aComponents.end())
                {
                    m_aComponents[spComponent->GetName()] = spComponent;
                }
                else
                {
                    GEKLOG(L"Component name already exists: %s", spComponent->GetName());
                }
            }

            return S_OK;
        });
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentSystemType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKComponentSystem> spSystem(pObject);
            if (spSystem)
            {
                m_aComponentSystems.push_back(spSystem);
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKPopulationManager::Destroy(void)
{
    Free();
    m_aComponents.clear();
    m_aComponentSystems.clear();
    GetContext()->RemoveCachedClass(CLSID_GEKPopulationManager);
}

STDMETHODIMP CGEKPopulationManager::Load(LPCWSTR pName, LPCWSTR pEntry)
{
    GEKFUNCTION(L"Name(%s), Entry(%s)", pName, pEntry);

    Free();

    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadBegin, std::placeholders::_1)));

    CLibXMLDoc kDocument;
    std::list<CLibXMLNode> aEntities;
    HRESULT hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\worlds\\%s.xml", pName));
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to Load Scene Document failed: 0x%08X", hRetVal);
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
                GEKLOG(L"Scene missing <population> node");
                hRetVal = E_INVALID;
            }
        }
        else
        {
            GEKLOG(L"Scene missing <world> node");
            hRetVal = E_INVALID;
        }
    }
    
    GEKENTITYID nPlayerID = 0;
    GEKLOG(L"Num. Entities Found: %d", aEntities.size());
    concurrency::parallel_for_each(aEntities.begin(), aEntities.end(), [&](CLibXMLNode &kEntityNode) -> void
    {
        GEKENTITYID nEntityID;
        if (SUCCEEDED(CreateEntity(nEntityID)))
        {
            CLibXMLNode &kComponentNode = kEntityNode.FirstChildElement();
            while (kComponentNode)
            {
                std::map<CStringW, CStringW> aParams;
                kComponentNode.ListAttributes([&aParams](LPCWSTR pName, LPCWSTR pValue) -> void
                {
                    aParams[pName] = pValue;
                });

                AddComponent(nEntityID, kComponentNode.GetType(), aParams);
                kComponentNode = kComponentNode.NextSiblingElement();
            };

            if (kEntityNode.HasAttribute(L"name") && kEntityNode.GetAttribute(L"name") == pEntry)
            {
                nPlayerID = nEntityID;
                AddComponent(nEntityID, L"viewer", { { L"fieldofview", FormatString(L"%f", _DEGTORAD(90.0f)) },
                                                     { L"minviewdistance", L"0.1" },
                                                     { L"maxviewdistance", L"150" },
                                                   });
                AddComponent(nEntityID, L"controller", {});

                IGEKViewManager *pViewManager = GetContext()->GetCachedClass<IGEKViewManager>(CLSID_GEKRenderManager);
                if (pViewManager)
                {
                    pViewManager->SetViewer(nEntityID);
                }
            }
        }
    });

    if (nPlayerID == 0)
    {
        hRetVal = E_INVALID;
        GEKLOG(L"Unable to locate entry point: %s", pEntry);
    }

    return CGEKObservable::CheckEvent(TGEKCheck<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadEnd, std::placeholders::_1, hRetVal)));
}

STDMETHODIMP_(void) CGEKPopulationManager::Free(void)
{
    m_aHitList.clear();
    m_aPopulation.clear();
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnFree, std::placeholders::_1)));
}

STDMETHODIMP_(void) CGEKPopulationManager::Update(float nGameTime, float nFrameTime)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPreUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPostUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    m_aHitList.clear();
}

STDMETHODIMP_(float3) CGEKPopulationManager::GetGravity(const float3 &nPosition)
{
    return float3(0.0f, -9.8331f, 0.0f);
}

STDMETHODIMP CGEKPopulationManager::CreateEntity(GEKENTITYID &nEntityID)
{
    static GEKENTITYID nNextEntityID = 0;
    m_aPopulation.push_back(++nNextEntityID);
    nEntityID = nNextEntityID;
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityCreated, std::placeholders::_1, nEntityID)));
    return S_OK;
}

STDMETHODIMP CGEKPopulationManager::DestroyEntity(const GEKENTITYID &nEntityID)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityDestroyed, std::placeholders::_1, nEntityID)));
    m_aHitList.push_back(nEntityID);
    return S_OK;
}

STDMETHODIMP CGEKPopulationManager::AddComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::map<CStringW, CStringW> &aParams)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        hRetVal = (*pIterator).second->AddComponent(nEntityID);
        if (SUCCEEDED(hRetVal))
        {
            for (auto kPair : aParams)
            {
                (*pIterator).second->SetProperty(nEntityID, kPair.first, kPair.second.GetString());
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnComponentAdded, std::placeholders::_1, nEntityID, pComponent)));
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationManager::RemoveComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnComponentRemoved, std::placeholders::_1, nEntityID, pComponent)));
        hRetVal = (*pIterator).second->RemoveComponent(nEntityID);
    }

    return hRetVal;
}

STDMETHODIMP_(bool) CGEKPopulationManager::HasComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    bool bReturn = false;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        bReturn = (*pIterator).second->HasComponent(nEntityID);
    }

    return bReturn;
}

STDMETHODIMP_(void) CGEKPopulationManager::ListProperties(const GEKENTITYID &nEntityID, LPCWSTR pComponent, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const
{
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        (*pIterator).second->ListProperties(nEntityID, OnProperty);
    }
}

STDMETHODIMP_(bool) CGEKPopulationManager::GetProperty(const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, GEKVALUE &kValue) const
{
    bool bReturn = false;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        bReturn = (*pIterator).second->GetProperty(nEntityID, pName, kValue);
    }

    return bReturn;
}

STDMETHODIMP_(bool) CGEKPopulationManager::SetProperty(const GEKENTITYID &nEntityID, LPCWSTR pComponent, LPCWSTR pName, const GEKVALUE &kValue)
{
    bool bReturn = false;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        bReturn = (*pIterator).second->SetProperty(nEntityID, pName, kValue);
    }

    return bReturn;
}

STDMETHODIMP_(void) CGEKPopulationManager::ListEntities(std::function<void(const GEKENTITYID &)> OnEntity)
{
    std::for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
    {
        OnEntity(nEntityID);
    });
}

STDMETHODIMP_(void) CGEKPopulationManager::ListComponentsEntities(const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity)
{
    std::for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
    {
        bool bEntityHasAllComponents = true;
        std::for_each(aComponents.begin(), aComponents.end(), [&](const CStringW &strComponent)-> void
        {
            if (!m_aComponents[strComponent]->HasComponent(nEntityID))
            {
                bEntityHasAllComponents = false;
            }
        });

        if (bEntityHasAllComponents)
        {
            OnEntity(nEntityID);
        }
    });
}
