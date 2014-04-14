#include "CGEKFactory.h"

// {CCD6D33D-53F6-4F40-9D12-7B31A4157BEA}
DEFINE_GUID(CLSID_GEKStaticModel, 0xccd6d33d, 0x53f6, 0x4f40, 0x9d, 0x12, 0x7b, 0x31, 0xa4, 0x15, 0x7b, 0xea);

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
    if (nGEKX == *(UINT32 *)"GEKX" && nType == 0 && nVersion == 2)
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKStaticModel, rIID, ppObject);
    }

    return hRetVal;
}
