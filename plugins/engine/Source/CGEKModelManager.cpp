#include "CGEKModelManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKModelManager)
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

STDMETHODIMP CGEKModelManager::Initialize(void)
{
    GEKFUNCTION(nullptr);

    HRESULT hRetVal = E_FAIL;
    m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
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

        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKModelManager::Destroy(void)
{
    m_aModels.clear();
}

STDMETHODIMP CGEKModelManager::Load(const UINT8 *pBuffer, LPCWSTR pParams, IUnknown **ppObject)
{
    GEKFUNCTION(L"Params(%s)", pParams);
    REQUIRE_RETURN(pBuffer, E_INVALIDARG);
    REQUIRE_RETURN(pParams, E_INVALIDARG);
    REQUIRE_RETURN(ppObject, E_INVALIDARG);

    HRESULT hRetVal = E_FAIL;
    for (auto &spFactory : m_aFactories)
    {
        CComPtr<IGEKModel> spModel;
        hRetVal = spFactory->Create(pBuffer, IID_PPV_ARGS(&spModel));
        if (spModel)
        {
            CComQIPtr<IGEKResource> spResource(spModel);
            if (spResource)
            {
                hRetVal = spResource->Load(pBuffer, pParams);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = spResource->QueryInterface(IID_PPV_ARGS(ppObject));
                    break;
                }
            }
            else
            {
                hRetVal = E_INVALID;
            }
        }
    }

    return hRetVal;
}
