#include "CGEKFactory.h"
#include "GEKModels.h"

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKStaticFactory)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKFactory)

CGEKFactory::CGEKFactory(void)
    : m_nNumInstances(50)
{
}

CGEKFactory::~CGEKFactory(void)
{
}

STDMETHODIMP CGEKFactory::Initialize(void)
{
    HRESULT hRetVal = E_FAIL;
    IGEKProgramManager *pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderManager);
    if (pProgramManager)
    {
        hRetVal = pProgramManager->LoadProgram(L"staticmodel", &m_spVertexProgram);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        IGEKVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem)
        {
            hRetVal = pVideoSystem->CreateBuffer(sizeof(IGEKModel::INSTANCE), m_nNumInstances, GEKVIDEO::BUFFER::DYNAMIC | GEKVIDEO::BUFFER::STRUCTURED_BUFFER | GEKVIDEO::BUFFER::RESOURCE, &m_spInstanceBuffer);
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKFactory::Destroy(void)
{
}

STDMETHODIMP CGEKFactory::Create(const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject)
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

STDMETHODIMP_(IUnknown *) CGEKFactory::GetVertexProgram(void)
{
    return m_spVertexProgram;
}

STDMETHODIMP_(IGEKVideoBuffer *) CGEKFactory::GetInstanceBuffer(void)
{
    return m_spInstanceBuffer;
}

STDMETHODIMP_(UINT32) CGEKFactory::GetNumInstances(void)
{
    return m_nNumInstances;
}