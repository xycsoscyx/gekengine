#include "CGEKStaticFactory.h"
#include "GEKModels.h"

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKStaticFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKStaticFactory)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticFactory)

CGEKStaticFactory::CGEKStaticFactory(void)
    : m_nNumInstances(50)
    , m_pRenderManager(nullptr)
    , m_pSceneManager(nullptr)
{
}

CGEKStaticFactory::~CGEKStaticFactory(void)
{
}

STDMETHODIMP CGEKStaticFactory::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKStaticFactory, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_pRenderManager = GetContext()->GetCachedClass<IGEKRenderManager>(CLSID_GEKRenderSystem);
        m_pSceneManager = GetContext()->GetCachedClass<IGEKSceneManager>(CLSID_GEKPopulationSystem);
        if (m_pRenderManager && m_pSceneManager)
        {
            hRetVal = CGEKObservable::AddObserver(m_pRenderManager, (IGEKRenderManagerObserver *)GetUnknown());
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = E_FAIL;
                CComQIPtr<IGEKProgramManager> spProgramManager(m_pRenderManager);
                if (spProgramManager != nullptr)
                {
                    hRetVal = spProgramManager->LoadProgram(L"staticmodel", &m_spVertexProgram);
                }
            }
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        IGEKVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem != nullptr)
        {
            hRetVal = pVideoSystem->CreateBuffer(sizeof(IGEKModel::INSTANCE), m_nNumInstances, GEKVIDEO::BUFFER::DYNAMIC | GEKVIDEO::BUFFER::STRUCTURED_BUFFER | GEKVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to CreateBuffer failed: 0x%08X", hRetVal);
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKStaticFactory::Destroy(void)
{
    CGEKObservable::RemoveObserver(m_pRenderManager, (IGEKRenderManagerObserver *)GetUnknown());
    GetContext()->RemoveCachedClass(CLSID_GEKStaticFactory);
}

STDMETHODIMP CGEKStaticFactory::Create(const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject)
{
    UINT32 nGEKX = *((UINT32 *)pBuffer);
    pBuffer += sizeof(UINT32);

    UINT16 nType = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    UINT16 nVersion = *((UINT16 *)pBuffer);

    HRESULT hRetVal = E_INVALIDARG;
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticModel, rIID, ppObject);
    }
    else  if (nGEKX == *(UINT32 *)"GEKX" && nType == 1 && nVersion == 2)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticCollision, rIID, ppObject);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKStaticFactory::OnPreRender(void)
{
}

STDMETHODIMP_(void) CGEKStaticFactory::OnCullScene(void)
{
    REQUIRE_VOID_RETURN(m_pSceneManager);

    m_pSceneManager->ListComponentsEntities({ L"transform", L"model" }, [&](const GEKENTITYID &nEntityID)->void
    {
        GEKVALUE kSource;
        GEKVALUE kParams;
        m_pSceneManager->GetProperty(nEntityID, L"model", L"source", kSource);
        m_pSceneManager->GetProperty(nEntityID, L"model", L"params", kParams);

        CComPtr<IUnknown> spModelUnknown;
        m_spModelManager->LoadModel(kSource.GetString(), kParams.GetString(), &spModelUnknown);
        if (spModelUnknown)
        {
            CComQIPtr<IGEKModel> spModel(spModelUnknown);
            if (spModel)
            {
                GEKVALUE kScale;
                m_pSceneManager->GetProperty(nEntityID, L"model", L"scale", kScale);
                kInstance.m_nScale = kScale.GetFloat3();

                GEKVALUE kPosition;
                GEKVALUE kRotation;
                m_pSceneManager->GetProperty(nEntityID, L"transform", L"position", kPosition);
                m_pSceneManager->GetProperty(nEntityID, L"transform", L"rotation", kRotation);
                kInstance.m_nMatrix = kRotation.GetQuaternion();
                kInstance.m_nMatrix.t = kPosition.GetFloat3();

                m_aVisibleModels[spModel].push_back(kInstance);
            }
        }
    });
}

STDMETHODIMP_(void) CGEKStaticFactory::OnDrawScene(void)
{
}

STDMETHODIMP_(void) CGEKStaticFactory::OnPostRender(void)
{
}
