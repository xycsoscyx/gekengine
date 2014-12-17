#include "CGEKPopulationSystem.h"
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
{
}

CGEKPopulationSystem::~CGEKPopulationSystem(void)
{
}

STDMETHODIMP CGEKPopulationSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKPopulationSystem, GetUnknown());
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

    return hRetVal;
}

STDMETHODIMP_(void) CGEKPopulationSystem::Destroy(void)
{
    Free();
    m_aComponents.clear();
    m_aComponentSystems.clear();
    GetContext()->RemoveCachedClass(CLSID_GEKPopulationSystem);
}

STDMETHODIMP CGEKPopulationSystem::LoadSystems(void)
{
    return GetContext()->CreateEachType(CLSID_GEKComponentSystemType, [&](IUnknown *pObject) -> HRESULT
    {
        CComQIPtr<IGEKComponentSystem> spSystem(pObject);
        if (spSystem)
        {
            m_aComponentSystems.push_back(spSystem);
        }

        return S_OK;
    });
}

STDMETHODIMP_(void) CGEKPopulationSystem::FreeSystems(void)
{
    m_aComponentSystems.clear();
}

STDMETHODIMP CGEKPopulationSystem::Load(LPCWSTR pName)
{
    GEKFUNCTION(L"Name(%s)", pName);

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
    std::for_each(aEntities.begin(), aEntities.end(), [&](CLibXMLNode &kEntityNode) -> void
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

                AddComponent(nEntityID, kComponentNode.GetType(), aParams);
                kComponentNode = kComponentNode.NextSiblingElement();
            };

#ifdef _DEBUG
            if (HasComponent(nEntityID, L"viewer"))
            {
                AddComponent(nEntityID, L"sprite", { { L"source", "camera" }, { L"size", "5" }, { L"color", L"1,1,1,1" } });
            }

            if (HasComponent(nEntityID, L"light"))
            {
                auto &kLight = ((IGEKSceneManager *)this)->GetComponent<GET_COMPONENT_DATA(light)>(nEntityID, L"light");
                float3 nColor = kLight.color.GetNormal();
                AddComponent(nEntityID, L"sprite", { { L"source", "light" }, { L"size", "3" }, { L"color", FormatString(L"%f,%f,%f,1.0", nColor.r, nColor.g, nColor.b).GetString() } });
            }
#endif
        }
    });

    return CGEKObservable::CheckEvent(TGEKCheck<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadEnd, std::placeholders::_1, hRetVal)));
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
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPreUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPostUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    m_aHitList.clear();
}

STDMETHODIMP_(float3) CGEKPopulationSystem::GetGravity(const float3 &nPosition) const
{
    return float3(0.0f, -9.8331f, 0.0f);
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

STDMETHODIMP CGEKPopulationSystem::AddComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::unordered_map<CStringW, CStringW> &aParams)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        hRetVal = (*pIterator).second->AddComponent(nEntityID);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = (*pIterator).second->DeSerialize(nEntityID, aParams);
            if (SUCCEEDED(hRetVal))
            {
                CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnComponentAdded, std::placeholders::_1, nEntityID, pComponent)));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationSystem::RemoveComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
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

STDMETHODIMP_(bool) CGEKPopulationSystem::HasComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    bool bReturn = false;
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        bReturn = (*pIterator).second->HasComponent(nEntityID);
    }

    return bReturn;
}

STDMETHODIMP_(LPVOID) CGEKPopulationSystem::GetComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
{
    auto pIterator = m_aComponents.find(pComponent);
    if (pIterator != m_aComponents.end())
    {
        return (*pIterator).second->GetComponent(nEntityID);
    }

    return nullptr;
}

STDMETHODIMP_(void) CGEKPopulationSystem::ListEntities(std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel)
{
    bParallel = false;
    if (bParallel)
    {
        concurrency::parallel_for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
        {
            OnEntity(nEntityID);
        });
    }
    else
    {
        std::for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
        {
            OnEntity(nEntityID);
        });
    }
}

STDMETHODIMP_(void) CGEKPopulationSystem::ListComponentsEntities(const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel)
{
    std::list<IGEKComponent *> aComponentList;
    std::for_each(aComponents.begin(), aComponents.end(), [&](const CStringW &strComponent)-> void
    {
        auto pIterator = m_aComponents.find(strComponent);
        if (pIterator != m_aComponents.end())
        {
            aComponentList.push_back((*pIterator).second);
        }
    });

    bParallel = false;
    if (bParallel)
    {
        concurrency::parallel_for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
        {
            bool bEntityHasAllComponents = true;
            std::for_each(aComponentList.begin(), aComponentList.end(), [&](IGEKComponent *pComponent)-> void
            {
                if (!pComponent->HasComponent(nEntityID))
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
    else
    {
        std::for_each(m_aPopulation.begin(), m_aPopulation.end(), [&](const GEKENTITYID &nEntityID)-> void
        {
            bool bEntityHasAllComponents = true;
            std::for_each(aComponentList.begin(), aComponentList.end(), [&](IGEKComponent *pComponent)-> void
            {
                if (!pComponent->HasComponent(nEntityID))
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
}
