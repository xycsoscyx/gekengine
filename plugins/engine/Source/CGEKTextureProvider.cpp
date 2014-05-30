#include "CGEKTextureProvider.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKTextureProvider)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceProvider)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKTextureProvider)

CGEKTextureProvider::CGEKTextureProvider(void)
{
}

CGEKTextureProvider::~CGEKTextureProvider(void)
{
}

STDMETHODIMP CGEKTextureProvider::Load(LPCWSTR pName, const UINT8 *pBuffer, UINT32 nBufferSize, IUnknown **ppObject)
{
    HRESULT hRetVal = E_FAIL;
    if (_wcsnicmp(pName, L"textures", 8) == 0)
    {
        IGEKVideoSystem *pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
        if (pVideoSystem)
        {
            CComPtr<IGEKVideoTexture> spTexture;
            hRetVal = pVideoSystem->LoadTexture(pBuffer, nBufferSize, &spTexture);
            if (spTexture)
            {
                hRetVal = spTexture->QueryInterface(IID_PPV_ARGS(ppObject));
            }
        }
    }

    return hRetVal;
}
