#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include "IGEKVideoSystem.h"
#include <atlbase.h>
#include <atlstr.h>

DECLARE_INTERFACE_IID_(IGEKInterfaceControl, IUnknown, "2DE1DB63-07D2-4A5C-995C-CFFE11C6C33E")
{
    STDMETHOD(AddChild)                 (THIS_ IGEKInterfaceControl *pChild) PURE;
    STDMETHOD(FindChild)                (THIS_ LPCWSTR pTitle, IGEKInterfaceControl **ppChild) PURE;
    STDMETHOD(ForEachChild)             (THIS_ std::function<void(IGEKInterfaceControl *)> OnChild) PURE;

    STDMETHOD_(LPCWSTR, GetClass)       (THIS) PURE;

    STDMETHOD_(LPCWSTR, GetTitle)       (THIS) PURE;
    STDMETHOD_(void, SetTitle)          (THIS_ LPCWSTR pTitle) PURE;

    STDMETHOD_(float2, GetPosition)     (THIS) PURE;
    STDMETHOD_(void, SetPosition)       (THIS_ const float2 &nPosition) PURE;

    STDMETHOD_(float2, GetSize)         (THIS) PURE;
    STDMETHOD_(void, SetSize)           (THIS_ const float2 &nSize) PURE;

    STDMETHOD_(bool, IsEnabled)         (THIS) PURE;
    STDMETHOD_(void, SetEnabled)        (THIS_ bool bEnabled) PURE;

    STDMETHOD_(bool, IsVisible)         (THIS) PURE;
    STDMETHOD_(void, SetVisible)        (THIS_ bool bVisible) PURE;

    STDMETHOD(SendMessage)              (THIS_ UINT32 nMessage, ...) PURE;
};

DECLARE_INTERFACE_IID_(IGEKInterfaceSystem, IUnknown, "D98F8768-BD77-43D1-AFBA-1A575F093938")
{
    STDMETHOD(SetContext)               (THIS_ IGEKVideoContext *pContext) PURE;

    STDMETHOD_(void, Print)             (THIS_ LPCWSTR pFont, float nSize, UINT32 nColor, const GEKVIDEO::RECT<float> &aLayoutRect, const GEKVIDEO::RECT<float> &kClipRect, LPCWSTR pFormat, ...) PURE;
};
