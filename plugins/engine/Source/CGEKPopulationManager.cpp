#include "CGEKPopulationManager.h"
#include "CGEKEntity.h"
#include <algorithm>

#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKPopulationManager)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKContextObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKPopulationManager)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKPopulationManager)

CGEKPopulationManager::CGEKPopulationManager(void)
    : m_bLevelLoaded(false)
{
}

CGEKPopulationManager::~CGEKPopulationManager(void)
{
}

STDMETHODIMP CGEKPopulationManager::OnRegistration(IUnknown *pObject)
{
    HRESULT hRetVal = S_OK;
    CComQIPtr<IGEKSceneManagerUser> spUser(pObject);
    if (spUser != nullptr)
    {
        hRetVal = spUser->Register(this);
    }


    return hRetVal;
}

STDMETHODIMP CGEKPopulationManager::Initialize(void)
{
    HRESULT hRetVal = CGEKObservable::AddObserver(GetContext(), this);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateEachType(CLSID_GEKComponentSystemType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKComponentSystem> spSystem(pObject);
            if (spSystem != nullptr)
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
    FreeScene();
    m_aComponentSystems.clear();
    CGEKObservable::RemoveObserver(GetContext(), this);
}

STDMETHODIMP CGEKPopulationManager::LoadScene(LPCWSTR pName, LPCWSTR pEntry)
{
    FreeScene();
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadBegin, std::placeholders::_1)));
    GetRenderManager()->BeginLoad();

    CLibXMLDoc kDocument;
    HRESULT hRetVal = kDocument.Load(FormatString(L"%%root%%\\data\\worlds\\%s.xml", pName));
    if (SUCCEEDED(hRetVal))
    {
        CLibXMLNode &kWorld = kDocument.GetRoot();
        if (kWorld.GetType().CompareNoCase(L"world") == 0)
        {
            hRetVal = GetRenderManager()->LoadWorld(kWorld.GetAttribute(L"source"), [&](float3 *pFace, IUnknown *pMaterial) -> HRESULT
            {
                return CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnStaticFace, std::placeholders::_1, pFace, pMaterial)));
            });

            if (SUCCEEDED(hRetVal))
            {
                CLibXMLNode &kPopulation = kWorld.FirstChildElement(L"population");
                if (kPopulation)
                {
                    CLibXMLNode &kEntity = kPopulation.FirstChildElement(L"entity");
                    while (kEntity)
                    {
                        hRetVal = AddEntity(kEntity);
                        if (SUCCEEDED(hRetVal))
                        {
                            kEntity = kEntity.NextSiblingElement(L"entity");
                        }
                        else
                        {
                            break;
                        }
                    };
                }
                else
                {
                    hRetVal = E_INVALID;
                }
            }
        }
        else
        {
            hRetVal = E_INVALID;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_INVALIDARG;
        auto pIterator = m_aPopulation.find(pEntry);
        if (pIterator != m_aPopulation.end())
        {
            CGEKEntity *pEntity = dynamic_cast<CGEKEntity *>((IGEKEntity *)(*pIterator).second);
            IGEKComponent *pTransform = ((*pIterator).second)->GetComponent(L"transform");
            if (pTransform)
            {
                CLibXMLDoc kDocument;
                kDocument.Create(L"player");
                CLibXMLNode &kPlayer = kDocument.GetRoot().CreateChildElement(L"entity");
                kPlayer.SetAttribute(L"name", L"player");
                kPlayer.SetAttribute(L"flags", pEntity->GetFlags());

                GEKVALUE kPosition;
                GEKVALUE kRotation;
                pTransform->GetProperty(L"position", kPosition);
                pTransform->GetProperty(L"rotation", kRotation);

                CLibXMLNode &kTransform = kPlayer.CreateChildElement(L"component");
                kTransform.SetAttribute(L"type", L"transform");
                kTransform.SetAttribute(L"position", kPosition.GetString());
                kTransform.SetAttribute(L"rotation", kRotation.GetString());

                CLibXMLNode &kViewer = kPlayer.CreateChildElement(L"component");
                kViewer.SetAttribute(L"type", L"viewer");
                kViewer.SetAttribute(L"fieldofview", L"%f", _DEGTORAD(90.0f));
                kViewer.SetAttribute(L"minviewdistance", L"0.1");
                kViewer.SetAttribute(L"maxviewdistance", L"100");

                CLibXMLNode &kScript = kPlayer.CreateChildElement(L"component");
                kScript.SetAttribute(L"type", L"script");
                kScript.SetAttribute(L"source", L"player");

                hRetVal = AddEntity(kPlayer);
                if (SUCCEEDED(hRetVal))
                {
                    pIterator = m_aPopulation.find(L"player");
                    if (pIterator == m_aPopulation.end())
                    {
                        hRetVal = E_ACCESSDENIED;
                    }
                    else
                    {
                        CComQIPtr<IGEKViewManager> spViewManager(GetRenderManager());
                        if (spViewManager)
                        {
                            hRetVal = spViewManager->SetViewer((*pIterator).second);
                        }
                    }
                }
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        m_bLevelLoaded = true;
    }

    GetRenderManager()->EndLoad(hRetVal);
    CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnLoadEnd, std::placeholders::_1, hRetVal)));

    return hRetVal;
}

STDMETHODIMP_(void) CGEKPopulationManager::FreeScene(void)
{
    if (m_bLevelLoaded)
    {
        m_bLevelLoaded = false;
        GetRenderManager()->FreeWorld();
        m_aInputHandlers.clear();
        m_aPopulation.clear();
        m_aHitList.clear();

        for (auto &pSystem : m_aComponentSystems)
        {
            pSystem->Clear();
        }
    }
}

STDMETHODIMP CGEKPopulationManager::OnInputEvent(LPCWSTR pName, const GEKVALUE &kValue)
{
    HRESULT hRetVal = S_OK;
    std::find_if(m_aInputHandlers.begin(), m_aInputHandlers.end(), [&](IGEKEntity *pEntity) -> bool
    {
        hRetVal = pEntity->OnEvent(L"input", pName, kValue);
        return FAILED(hRetVal);
    });

    return hRetVal;
}

STDMETHODIMP_(void) CGEKPopulationManager::Update(float nGameTime, float nFrameTime)
{
    if (m_bLevelLoaded)
    {
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPreUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnPostUpdate, std::placeholders::_1, nGameTime, nFrameTime)));
        for (auto &pEntity : m_aHitList)
        {
            auto pIterator = std::find_if(m_aPopulation.begin(), m_aPopulation.end(), [pEntity](std::map<GEKHASH, CComPtr<IGEKEntity>>::value_type &kPair) -> bool
            {
                return (kPair.second == pEntity);
            });

            if (pIterator != m_aPopulation.end())
            {
                m_aInputHandlers.erase(std::remove_if(m_aInputHandlers.begin(), m_aInputHandlers.end(), [&](IGEKEntity *pInputEntity) -> bool
                {
                    return (pEntity == pInputEntity);
                }), m_aInputHandlers.end());

                dynamic_cast<CGEKEntity *>(pEntity)->OnEntityDestroyed();
                CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnEntityDestroyed, std::placeholders::_1, pEntity)));
                for (auto &pSystem : m_aComponentSystems)
                {
                    pSystem->Destroy(pEntity);
                }

                m_aPopulation.erase(pIterator);
            }
        }

        m_aHitList.clear();
    }
}

STDMETHODIMP_(void) CGEKPopulationManager::Render(void)
{
    if (m_bLevelLoaded)
    {
        CGEKObservable::SendEvent(TGEKEvent<IGEKSceneObserver>(std::bind(&IGEKSceneObserver::OnRender, std::placeholders::_1)));
    }
}

STDMETHODIMP CGEKPopulationManager::AddEntity(CLibXMLNode &kEntity)
{
    HRESULT hRetVal = E_OUTOFMEMORY;
    CStringW strName = kEntity.GetAttribute(L"name");
    if (strName.IsEmpty())
    {
        static int nUnNamedCount = 0;
        strName.Format(L"entity_%d", nUnNamedCount++);
    }

    CStringW strFlags = kEntity.GetAttribute(L"flags");
    auto pIterator = m_aPopulation.find(strName.GetString());
    if (pIterator != m_aPopulation.end())
    {
        hRetVal = E_ACCESSDENIED;
    }
    else
    {
        CComPtr<CGEKEntity> spEntity(new CGEKEntity(strFlags));
        if (spEntity)
        {
            CLibXMLNode &kComponent = kEntity.FirstChildElement(L"component");
            while (kComponent)
            {
                CComPtr<IGEKComponent> spComponent;
                std::find_if(m_aComponentSystems.begin(), m_aComponentSystems.end(), [&](IGEKComponentSystem *pSystem) -> bool
                {
                    return SUCCEEDED(pSystem->Create(kComponent, spEntity, &spComponent));
                });

                if (spComponent)
                {
                    spEntity->AddComponent(spComponent);
                }

                kComponent = kComponent.NextSiblingElement(L"component");
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
