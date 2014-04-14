#include "CGEKFactory.h"

// {5A5F6DA2-F445-4E52-830C-5A7464BA35EC}
DEFINE_GUID(CLSID_GEKStaticWorld, 0x5a5f6da2, 0xf445, 0x4e52, 0x83, 0xc, 0x5a, 0x74, 0x64, 0xba, 0x35, 0xec);

BEGIN_INTERFACE_LIST(CGEKFactory)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKFactory)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKFactory)

CGEKFactory::CGEKFactory(void)
{
}

CGEKFactory::~CGEKFactory(void)
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
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 10 && nVersion == 1)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticWorld, rIID, ppObject);
    }

    return hRetVal;
}
