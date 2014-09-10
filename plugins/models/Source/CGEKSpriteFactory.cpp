#include "CGEKSpriteFactory.h"
#include "GEKModels.h"

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKSpriteFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKSpriteFactory)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKSpriteFactory)

CGEKSpriteFactory::CGEKSpriteFactory(void)
    : m_nNumInstances(50)
{
}

CGEKSpriteFactory::~CGEKSpriteFactory(void)
{
}

STDMETHODIMP CGEKSpriteFactory::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKStaticFactory, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        IGEKProgramManager *pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderSystem);
        if (pProgramManager != nullptr)
        {
            hRetVal = pProgramManager->LoadProgram(L"spritemodel", &m_spVertexProgram);
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

STDMETHODIMP_(void) CGEKSpriteFactory::Destroy(void)
{
    GetContext()->RemoveCachedClass(CLSID_GEKStaticFactory);
}

STDMETHODIMP CGEKSpriteFactory::Create(const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject)
{
    HRESULT hRetVal = E_INVALIDARG;

    return hRetVal;
}

STDMETHODIMP_(IUnknown *) CGEKSpriteFactory::GetVertexProgram(void)
{
    return m_spVertexProgram;
}

STDMETHODIMP_(IGEKVideoBuffer *) CGEKSpriteFactory::GetInstanceBuffer(void)
{
    return m_spInstanceBuffer;
}

STDMETHODIMP_(UINT32) CGEKSpriteFactory::GetNumInstances(void)
{
    return m_nNumInstances;
}