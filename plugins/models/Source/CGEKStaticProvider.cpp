#include "CGEKStaticProvider.h"
#include "GEKModels.h"

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKStaticProvider)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceProvider)
    INTERFACE_LIST_ENTRY_COM(IGEKStaticProvider)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKStaticProvider)

CGEKStaticProvider::CGEKStaticProvider(void)
    : m_nNumInstances(50)
{
}

CGEKStaticProvider::~CGEKStaticProvider(void)
{
}

STDMETHODIMP CGEKStaticProvider::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKStaticProvider, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        IGEKProgramManager *pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderManager);
        if (pProgramManager != nullptr)
        {
            hRetVal = pProgramManager->LoadProgram(L"staticmodel", &m_spVertexProgram);
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

STDMETHODIMP_(void) CGEKStaticProvider::Destroy(void)
{
    GetContext()->RemoveCachedClass(CLSID_GEKStaticProvider);
}

STDMETHODIMP CGEKStaticProvider::Load(LPCWSTR pName, const UINT8 *pBuffer, UINT32 nBufferSize, IUnknown **ppObject)
{
    UINT32 nGEKX = *((UINT32 *)pBuffer);
    pBuffer += sizeof(UINT32);

    UINT16 nType = *((UINT16 *)pBuffer);
    pBuffer += sizeof(UINT16);

    UINT16 nVersion = *((UINT16 *)pBuffer);

    HRESULT hRetVal = E_INVALIDARG;
    CComPtr<IGEKStaticData> spData;
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticModel, IID_PPV_ARGS(&spData));
    }
    else  if (nGEKX == *(UINT32 *)"GEKX" && nType == 1 && nVersion == 2)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticCollision, IID_PPV_ARGS(&spData));
    }

    if (spData)
    {
        hRetVal = spData->Load(pBuffer, nBufferSize);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = spData->QueryInterface(IID_PPV_ARGS(ppObject));
        }
    }

    return hRetVal;
}

STDMETHODIMP_(IUnknown *) CGEKStaticProvider::GetVertexProgram(void)
{
    return m_spVertexProgram;
}

STDMETHODIMP_(IGEKVideoBuffer *) CGEKStaticProvider::GetInstanceBuffer(void)
{
    return m_spInstanceBuffer;
}

STDMETHODIMP_(UINT32) CGEKStaticProvider::GetNumInstances(void)
{
    return m_nNumInstances;
}