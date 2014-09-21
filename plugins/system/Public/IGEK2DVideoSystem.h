#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include <atlbase.h>
#include <atlstr.h>

namespace GEK2DVIDEO
{
    namespace FONT
    {
        enum STYLE
        {
            NORMAL              = 0,
            ITALIC,
        };
    };

    namespace GRADIENT
    {
        struct STOP
        {
            float m_nPosition;
            float4 m_nColor;
        };
    };
};

DECLARE_INTERFACE_IID_(IGEK2DVideoGeometry, IUnknown, "4CA2D559-66C1-46F3-ADFF-9B919AAB4575")
{
    STDMETHOD(Open)                         (THIS) PURE;
    STDMETHOD(Close)                        (THIS) PURE;

    STDMETHOD_(void, Begin)                 (THIS_ const float2 &nPoint, bool bFilled) PURE;
    STDMETHOD_(void, End)                   (THIS_ bool bOpenEnded) PURE;

    STDMETHOD_(void, AddLine)               (THIS_ const float2 &nPoint) PURE;
    STDMETHOD_(void, AddBezier)             (THIS_ const float2 &nPoint1, const float2 &nPoint2, const float2 &nPoint3) PURE;

    STDMETHOD(Widen)                        (THIS_ float nWidth, float nTolerance, IGEK2DVideoGeometry **ppGeometry) PURE;
};

DECLARE_INTERFACE_IID_(IGEK2DVideoSystem, IUnknown, "D3B65773-4EB1-46F8-A38D-009CA43CE77F")
{
    STDMETHOD(CreateBrush)                  (THIS_ const float4 &nColor, IUnknown **ppBrush) PURE;
    STDMETHOD(CreateBrush)                  (THIS_ const std::vector<GEK2DVIDEO::GRADIENT::STOP> &aStops, const trect<float> &kRect, IUnknown **ppBrush) PURE;

    STDMETHOD(CreateFont)                   (THIS_ LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize, IUnknown **ppFont) PURE;

    STDMETHOD(CreateGeometry)               (THIS_ IGEK2DVideoGeometry **ppGeometry) PURE;

    STDMETHOD_(void, SetTransform)          (THIS_ const float3x2 &nTransform) PURE;

    STDMETHOD_(void, DrawText)              (THIS_ const trect<float> &kLayout, IUnknown *pFont, IUnknown *pBrush, LPCWSTR pMessage, ...) PURE;

    STDMETHOD_(void, DrawRectangle)         (THIS_ const trect<float> &kRect, IUnknown *pBrush, bool bFilled) PURE;
    STDMETHOD_(void, DrawRectangle)         (THIS_ const trect<float> &kRect, const float2 &nRadius, IUnknown *pBrush, bool bFilled) PURE;

    STDMETHOD_(void, DrawGeometry)          (THIS_ IGEK2DVideoGeometry *pGeometry, IUnknown *pBrush, bool bFilled) PURE;

    STDMETHOD_(void, Begin)                 (THIS) PURE;
    STDMETHOD(End)                          (THIS) PURE;
};
