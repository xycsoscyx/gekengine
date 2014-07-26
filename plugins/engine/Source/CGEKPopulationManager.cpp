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
    
    GEKLOG(L"Num. Entities Found: %d", aEntities.size());
    concurrency::parallel_for_each(aEntities.begin(), aEntities.end(), [&](CLibXMLNode &kEntityNode) -> void
    {
        GEKENTITYID nEntityID;
        if (SUCCEEDED(CreateEntity(nEntityID)))
        {
            CLibXMLNode &kComponentNode = kEntityNode.FirstChildElement();
            while (kComponentNode)
            {
                auto pIterator = m_aComponents.find(kComponentNode.GetType());
                if (pIterator != m_aComponents.end())
                {
                    CComPtr<IGEKComponent> spComponent;
                    if (SUCCEEDED((*pIterator).second->AddComponent(nEntityID)))
                    {
                        kComponentNode.ListAttributes([&](LPCWSTR pName, LPCWSTR pValue) -> void
                        {
                            (*pIterator).second->SetProperty(nEntityID, pName, pValue);
                        });
                    }
                }

                kComponentNode = kComponentNode.NextSiblingElement();
            };
        }
    });

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
    return S_OK;
}

STDMETHODIMP CGEKPopulationManager::DestroyEntity(const GEKENTITYID &nEntityID)
{
    m_aHitList.push_back(nEntityID);
    return S_OK;
}

STDMETHODIMP CGEKPopulationManager::AddComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    return m_aComponents[pComponent]->AddComponent(nEntityID);
}

STDMETHODIMP CGEKPopulationManager::RemoveComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    return m_aComponents[pComponent]->RemoveComponent(nEntityID);
}

STDMETHODIMP_(bool) CGEKPopulationManager::HasComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    return m_aComponents[pComponent]->HasComponent(nEntityID);
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

STDMETHODIMP_(IGEKComponent *) CGEKPopulationManager::GetComponent(LPCWSTR pComponent)
{
    return m_aComponents[pComponent];
}
