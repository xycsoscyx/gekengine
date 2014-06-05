#include "CGEKModelManager.h"
#include "IGEKRenderFilter.h"
#include "CGEKProperties.h"
#include <windowsx.h>
#include <algorithm>
#include <atlpath.h>
#include <ppl.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKModelManager)
    INTERFACE_LIST_ENTRY_COM(IGEKSceneObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKModelManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKModelManager)

CGEKModelManager::CGEKModelManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
{
}

CGEKModelManager::~CGEKModelManager(void)
{
}

STDMETHODIMP_(void) CGEKModelManager::OnBeginLoad(void)
{
    m_aModels.clear();
}

STDMETHODIMP CGEKModelManager::OnLoadEnd(HRESULT hRetVal)
{
    return hRetVal;
}

STDMETHODIMP_(void) CGEKModelManager::OnFree(void)
{
    m_aModels.clear();
}

STDMETHODIMP CGEKModelManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKModelManager, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
        m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
        {
            hRetVal = S_OK;
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        GetContext()->CreateEachType(CLSID_GEKFactoryType, [&](IUnknown *pObject) -> HRESULT
        {
            CComQIPtr<IGEKFactory> spFactory(pObject);
            if (spFactory)
            {
                m_aFactories.push_back(spFactory);
            }

            return S_OK;
        });
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKModelManager::Destroy(void)
{
    m_aModels.clear();
    GetContext()->RemoveCachedObserver(CLSID_GEKPopulationManager, (IGEKSceneObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKModelManager);
}

STDMETHODIMP CGEKModelManager::LoadCollision(LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision)
{
    GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);
    REQUIRE_RETURN(ppCollision, E_INVALIDARG);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);

    std::vector<UINT8> aBuffer;
    HRESULT hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.collision.gek", pName), aBuffer);
    if (SUCCEEDED(hRetVal))
    {
        for (auto &spFactory : m_aFactories)
        {
            hRetVal = spFactory->Create(&aBuffer[0], IID_PPV_ARGS(ppCollision));
            if (*ppCollision)
            {
                CComQIPtr<IGEKResource> spResource(*ppCollision);
                if (spResource)
                {
                    hRetVal = spResource->Load(&aBuffer[0], pParams);
                    if (SUCCEEDED(hRetVal))
                    {
                        break;
                    }
                }
                else
                {
                    hRetVal = E_INVALID;
                }
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP CGEKModelManager::LoadModel(LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel)
{
    concurrency::critical_section::scoped_lock kLock(m_kCritical);
    GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);
    REQUIRE_RETURN(ppModel, E_INVALIDARG);
    REQUIRE_RETURN(pName, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    auto pIterator = m_aModels.find(FormatString(L"%s|%s", pName, pParams));
    if (pIterator != m_aModels.end())
    {
        hRetVal = ((*pIterator).second)->QueryInterface(IID_PPV_ARGS(ppModel));
    }
    else
    {
        std::vector<UINT8> aBuffer;
        hRetVal = GEKLoadFromFile(FormatString(L"%%root%%\\data\\models\\%s.model.gek", pName), aBuffer);
        if (SUCCEEDED(hRetVal))
        {
            for (auto &spFactory : m_aFactories)
            {
                CComPtr<IGEKModel> spModel;
                hRetVal = spFactory->Create(&aBuffer[0], IID_PPV_ARGS(&spModel));
                if (spModel)
                {
                    CComQIPtr<IGEKResource> spResource(spModel);
                    if (spResource)
                    {
                        hRetVal = spResource->Load(&aBuffer[0], pParams);
                        if (SUCCEEDED(hRetVal))
                        {
                            m_aModels[FormatString(L"%s|%s", pName, pParams)] = spModel;
                            hRetVal = spModel->QueryInterface(IID_PPV_ARGS(ppModel));
                            break;
                        }
                    }
                    else
                    {
                        hRetVal = E_INVALID;
                    }
                }
            }
        }
    }

    return hRetVal;
}
