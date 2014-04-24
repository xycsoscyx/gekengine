#include "CGEKFactory.h"
#include "GEKModels.h"

BEGIN_INTERFACE_LIST(CGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKVideoSystemUser)
    INTERFACE_LIST_ENTRY_COM(IGEKProgramManagerUser)
    INTERFACE_LIST_ENTRY_COM(IGEKContextObserver)
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
    HRESULT hRetVal = CGEKObservable::AddObserver(GetContext(), (IGEKContextObserver *)this);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetProgramManager()->LoadProgram(L"staticmodel", &m_spVertexProgram);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetVideoSystem()->CreateVertexBuffer(sizeof(IGEKModel::INSTANCE), m_nNumInstances, &m_spInstanceBuffer);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKFactory::Destroy(void)
{
    CGEKObservable::RemoveObserver(GetContext(), (IGEKContextObserver *)this);
}

STDMETHODIMP CGEKFactory::OnRegistration(IUnknown *pObject)
{
    HRESULT hRetVal = S_OK;
    CComQIPtr<IGEKStaticFactoryUser> spuser(pObject);
    if (spuser != nullptr)
    {
        hRetVal = spuser->Register(this);
    }

    return hRetVal;
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

STDMETHODIMP_(IGEKVideoVertexBuffer *) CGEKFactory::GetInstanceBuffer(void)
{
    return m_spInstanceBuffer;
}

STDMETHODIMP_(UINT32) CGEKFactory::GetNumInstances(void)
{
    return m_nNumInstances;
}