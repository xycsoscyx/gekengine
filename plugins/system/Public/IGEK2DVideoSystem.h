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
};

DECLARE_INTERFACE_IID_(IGEK2DVideoSystem, IUnknown, "D3B65773-4EB1-46F8-A38D-009CA43CE77F")
{
    STDMETHOD(CreateFont)                   (THIS_ LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize, IUnknown **ppFont) PURE;

    STDMETHOD_(void, Print)                 (THIS_ const trect<float> &kLayout, IUnknown *pFont, IUnknown *pBrush, LPCWSTR pMessage, ...) PURE;

    STDMETHOD(CreateBrush)                  (THIS_ const float4 &nColor, IUnknown **ppBrush) PURE;

    STDMETHOD_(void, DrawRectangle)         (THIS_ const trect<float> &kRect, IUnknown *pBrush, bool bFilled) PURE;
    STDMETHOD_(void, DrawRectangle)         (THIS_ const trect<float> &kRect, const float2 &nRadius, IUnknown *pBrush, bool bFilled) PURE;

    STDMETHOD_(void, Begin)                 (THIS) PURE;
    STDMETHOD(End)                          (THIS) PURE;
};
