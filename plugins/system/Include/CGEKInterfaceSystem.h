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

    IGEKVideoSystem *m_pVideoSystem;
    IGEKVideoContext *m_pVideoContext;
    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IUnknown> m_spPixelProgram;
    CComPtr<IUnknown> m_spPointSampler;
    CComPtr<IUnknown> m_spBlendStates;
    CComPtr<IUnknown> m_spRenderStates;
    CComPtr<IUnknown> m_spDepthStates;
    CComPtr<IGEKVideoBuffer> m_spOrthoBuffer;
    CComPtr<IGEKVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEKVideoBuffer> m_spIndexBuffer;

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
    STDMETHOD_(void, DrawSprite)        (THIS_ const GEKVIDEO::RECT<float> &kRect, const GEKVIDEO::RECT<float> &kTexCoords, const float4 &nColor, IGEKVideoTexture *pTexture);
};
