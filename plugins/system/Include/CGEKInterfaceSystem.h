#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "IGEKInterfaceSystem.h"
#include <FW1FontWrapper.h>
#include <D3D11.h>

class CGEKInterfaceSystem : public CGEKUnknown
                          , public IGEKInterfaceSystem
{
protected:
    CComQIPtr<ID3D11Device> m_spDevice;
    CComQIPtr<ID3D11DeviceContext> m_spDeviceContext;
    CComPtr<IFW1Factory> m_spFontFactory;
    CComPtr<IFW1FontWrapper> m_spFontWrapper;

public:
    CGEKInterfaceSystem(void);
    virtual ~CGEKInterfaceSystem(void);
    DECLARE_UNKNOWN(CGEKInterfaceSystem);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKInterfaceSystem
    STDMETHOD(SetContext)               (THIS_ IGEKVideoContext *pContext);
    STDMETHOD_(void, Print)             (THIS_ LPCWSTR pFont, float nSize, UINT32 nColor, const GEKVIDEO::RECT<float> &aLayoutRect, const GEKVIDEO::RECT<float> &kClipRect, LPCWSTR pFormat, ...);
};
