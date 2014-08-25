#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include "IGEKVideoSystem.h"
#include <atlbase.h>
#include <atlstr.h>

DECLARE_INTERFACE_IID_(IGEKInterfaceSystem, IUnknown, "D98F8768-BD77-43D1-AFBA-1A575F093938")
{
    STDMETHOD(SetContext)               (THIS_ IGEKVideoContext *pContext) PURE;

    STDMETHOD_(void, Print)             (THIS_ LPCWSTR pFont, float nSize, UINT32 nColor, const GEKVIDEO::RECT<float> &aLayoutRect, const GEKVIDEO::RECT<float> &kClipRect, LPCWSTR pFormat, ...) PURE;

    STDMETHOD_(void, DrawSprite)        (THIS_ const GEKVIDEO::RECT<float> &kRect, const GEKVIDEO::RECT<float> &kTexCoords, const float4 &nColor, IGEKVideoTexture *pTexture) PURE;
};
