#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>

DECLARE_INTERFACE(IGEKVideoTexture);

DECLARE_INTERFACE_IID_(IGEKMaterial, IUnknown, "819CA201-F652-4183-B29D-BB71BB15810E")
{
    STDMETHOD_(void, SetPass)                       (THIS_ LPCWSTR pPass) PURE;
    STDMETHOD_(void, SetAlbedoMap)                  (THIS_ IUnknown *pTexture) PURE;
    STDMETHOD_(void, SetNormalMap)                  (THIS_ IUnknown *pTexture) PURE;
    STDMETHOD_(void, SetInfoMap)                    (THIS_ IUnknown *pTexture) PURE;

    STDMETHOD_(LPCWSTR, GetPass)                    (THIS) PURE;
    STDMETHOD_(IUnknown *, GetAlbedoMap)            (THIS)PURE;
    STDMETHOD_(IUnknown *, GetNormalMap)            (THIS)PURE;
    STDMETHOD_(IUnknown *, GetInfoMap)              (THIS)PURE;
};
