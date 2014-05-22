#include "CGEKPopulationManager.h"
#include "CGEKEntity.h"
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
    GEKFUNCTION();
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKPopulationManager, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentSystemType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKComponentSystem> spSystem(pObject);
            if (spSystem)
            {
                GEKLOG(L"Component System Found: %s", spSystem->GetType());
                m_aComponentSystems[spSystem->GetType()] = spSystem;
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKPopulationManager::Destroy(void)
{
    Free();
    m_aComponentSystems.clear();
    GetContext()->RemoveCachedClass(CLSID_GEKPopulationManager);
}

STDMETHODIMP CGEKPopulationManager::LoadScene(LPCWSTR pName, LPCWSTR pEntry)
{
    GEKFUNCTION();

    GEKLOG(L"Loading Scene: %s (%s)", pName, pEntry);

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

    concurrency::critical_section kCritical;
    concurrency::parallel_for_each(aEntities.begin(), aEntities.end(), [&](CLibXMLNode &kEntityNode) -> void
    {
        HRESULT hAddRetVal = AddEntity(kEntityNode);
        if (FAILED(hAddRetVal))
        {
            kCritical.lock();
            hRetVal = hAddRetVal;
            kCritical.unlock();
        }
    });

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_INVALIDARG;
        auto pIterator = m_aPopulation.find(pEntry);
        GEKRESULT(pIterator != m_aPopulation.end(), L"Unable to locate scene entry entity: %s", pEntry);
        if (pIterator != m_aPopulation.end())
        {
            CGEKEntity *pEntity = dynamic_cast<CGEKEntity *>((IGEKEntity *)(*pIterator).second);
            IGEKComponent *pTransform = ((*pIterator).second)->GetComponent(L"transform");
            GEKRESULT(pTransform, L"Unable to locate scene entry transform component: %s", pEntry);
            if (pTransform != nullptr)
            {
                CLibXMLDoc kDocument;
                kDocument.Create(L"player");
                CLibXMLNode &kPlayerNode = kDocument.GetRoot().CreateChildElement(L"entity");
                kPlayerNode.SetAttribute(L"name", L"player");
                kPlayerNode.SetAttribute(L"flags", pEntity->GetFlags());

                GEKVALUE kPosition;
                GEKVALUE kRotation;
                pTransform->GetProperty(L"position", kPosition);
                pTransform->GetProperty(L"rotation", kRotation);

                CLibXMLNode &kTransformNode = kPlayerNode.CreateChildElement(L"component");
                kTransformNode.SetAttribute(L"type", L"transform");
                kTransformNode.SetAttribute(L"position", kPosition.GetString());
                kTransformNode.SetAttribute(L"rotation", kRotation.GetString());

                CLibXMLNode &kViewerNode = kPlayerNode.CreateChildElement(L"component");
                kViewerNode.SetAttribute(L"type", L"viewer");
                kViewerNode.SetAttribute(L"fieldofview", L"%f", _DEGTORAD(90.0f));
                kViewerNode.SetAttribute(L"minviewdistance", L"0.1");
                kViewerNode.SetAttribute(L"maxviewdistance", L"150");

                CLibXMLNode &kScriptNode = kPlayerNode.CreateChildElement(L"component");
                kScriptNode.SetAttribute(L"type", L"logic");
                kScriptNode.SetAttribute(L"state", L"default_player_state");

                hRetVal = AddEntity(kPlayerNode);
                if (SUCCEEDED(hRetVal))
                {
                    pIterator = m_aPopulation.find(L"player");
                    GEKRESULT(pIterator == m_aPopulation.end(), L"Player entity already exists in scene");
                    if (pIterator == m_aPopulation.end())
                    {
                        hRetVal = E_ACCESSDENIED;
                    }
                }
            }
        }
    }

    return CGEKObservable::CheckEvent(TGEKCheck<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadEnd, std::placeholders::_1, hRetVal)));
}

STDMETHODIMP_(void) CGEKPopulationManager::Free(void)
{
    m_aInputHandlers.clear();
    m_aPopulation.clear();
    m_aHitList.clear();

    for (auto &kPair : m_aComponentSystems)
    {
        kPair.second->Clear();
    }
}

STDMETHODIMP_(void) CGEKPopulationManager::OnInputEvent(LPCWSTR pName, const GEKVALUE &kValue)
{
    for (auto &pEntity : m_aInputHandlers)
    {
        pEntity->OnEvent(L"input", pName, kValue);
    };
}

STDMETHODIMP_(void) CGEKPopulationManager::Update(float nGameTime, float nFrameTime)
{
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPreUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPostUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
    for (auto &pEntity : m_aHitList)
    {
        auto pIterator = std::find_if(m_aPopulation.begin(), m_aPopulation.end(), [pEntity](std::map<GEKHASH, CComPtr<IGEKEntity>>::value_type &kPair) -> bool
        {
            return (kPair.second.IsEqualObject(pEntity));
        });

        if (pIterator != m_aPopulation.end())
        {
            m_aInputHandlers.erase(std::remove_if(m_aInputHandlers.begin(), m_aInputHandlers.end(), [&](IGEKEntity *pInputEntity) -> bool
            {
                return (pEntity == pInputEntity);
            }), m_aInputHandlers.end());

            dynamic_cast<CGEKEntity *>(pEntity)->OnEntityDestroyed();
            CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityDestroyed, std::placeholders::_1, pEntity)));
            for (auto &kPair : m_aComponentSystems)
            {
                kPair.second->Destroy(pEntity);
            }

            m_aPopulation.unsafe_erase(pIterator);
        }
    }

    m_aHitList.clear();
}

STDMETHODIMP CGEKPopulationManager::AddEntity(CLibXMLNode &kEntityNode)
{
    GEKFUNCTION();
    HRESULT hRetVal = E_OUTOFMEMORY;
    CStringW strName = kEntityNode.GetAttribute(L"name");
    if (strName.IsEmpty())
    {
        static int nUnNamedCount = 0;
        strName.Format(L"entity_%d", nUnNamedCount++);
    }

    CStringW strFlags = kEntityNode.GetAttribute(L"flags");
    auto pIterator = m_aPopulation.find(strName.GetString());
    GEKRESULT(pIterator != m_aPopulation.end(), L"Entity already exists in scene: %s", strName.GetString());
    if (pIterator != m_aPopulation.end())
    {
        hRetVal = E_ACCESSDENIED;
    }
    else
    {
        CComPtr<CGEKEntity> spEntity(new CGEKEntity(strFlags));
        GEKRESULT(spEntity, L"Call to new failed to allocate instance");
        if (spEntity)
        {
            CLibXMLNode &kComponentNode = kEntityNode.FirstChildElement(L"component");
            while (kComponentNode)
            {
                if (kComponentNode.HasAttribute(L"type"))
                {
                    CStringW strType = kComponentNode.GetAttribute(L"type");
                    strType.MakeLower();

                    auto pIterator = m_aComponentSystems.find(strType);
                    if (pIterator != m_aComponentSystems.end())
                    {
                        CComPtr<IGEKComponent> spComponent;
                        (*pIterator).second->Create(kComponentNode, spEntity, &spComponent);
                        if (spComponent)
                        {
                            spEntity->AddComponent((*pIterator).second->GetType(), spComponent);
                        }
                    }
                }

                kComponentNode = kComponentNode.NextSiblingElement(L"component");
            };

            if (SUCCEEDED(spEntity->OnEntityCreated()))
            {
                hRetVal = spEntity->QueryInterface(IID_PPV_ARGS(&m_aPopulation[strName.GetString()]));
                if (strFlags.Find(L"input") >= 0)
                {
                    m_aInputHandlers.push_back(spEntity);
                }

                CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityAdded, std::placeholders::_1, (IGEKEntity *)spEntity)));
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationManager::FindEntity(LPCWSTR pName, IGEKEntity **ppEntity)
{
    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aPopulation.find(pName);
    if (pIterator != m_aPopulation.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppEntity));
    }

    return hRetVal;
}

STDMETHODIMP CGEKPopulationManager::DestroyEntity(IGEKEntity *pEntity)
{
    m_aHitList.push_back(pEntity);
    return S_OK;
}

STDMETHODIMP_(float3) CGEKPopulationManager::GetGravity(const float4 &nGravity)
{
    return float3(0.0f, -9.81f, 0.0f);
}

STDMETHODIMP_(void) CGEKPopulationManager::GetVisible(const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities)
{
    for (auto &kPair : m_aComponentSystems)
    {
        kPair.second->GetVisible(kFrustum, aVisibleEntities);
    }
}
