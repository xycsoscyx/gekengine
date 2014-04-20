#pragma once

#include "GEKContext.h"
#include "IGEKMaterial.h"

class CGEKMaterial : public CGEKUnknown
                   , public IGEKMaterial
{
private:
    CStringW m_strPass;
    float4 m_nParams;
    CComPtr<IUnknown> m_spAlbedoMap;
    CComPtr<IUnknown> m_spNormalMap;
    CComPtr<IUnknown> m_spInfoMap;

public:
    CGEKMaterial(void);
    virtual ~CGEKMaterial(void);
    DECLARE_UNKNOWN(CGEKMaterial);

    // IGEKMaterial
    STDMETHOD_(void, SetPass)                       (THIS_ LPCWSTR pPass);
    STDMETHOD_(void, SetParams)                     (THIS_ const float4 &nParams);
    STDMETHOD_(void, SetAlbedoMap)                  (THIS_ IUnknown *pTexture);
    STDMETHOD_(void, SetNormalMap)                  (THIS_ IUnknown *pTexture);
    STDMETHOD_(void, SetInfoMap)                    (THIS_ IUnknown *pTexture);
    STDMETHOD_(LPCWSTR, GetPass)                    (THIS);
    STDMETHOD_(float4, GetParams)                   (THIS);
    STDMETHOD_(IUnknown *, GetAlbedoMap)            (THIS);
    STDMETHOD_(IUnknown *, GetNormalMap)            (THIS);
    STDMETHOD_(IUnknown *, GetInfoMap)              (THIS);
};