#pragma once

#include <Windows.h>

namespace Gek
{
    DECLARE_INTERFACE_IID(Engine, "D5A3F1DC-BD5B-49ED-AEE1-FF3C525412D2") : virtual public IUnknown
    {
        STDMETHOD(initialize)               (THIS_ HWND window) PURE;

        STDMETHOD_(LRESULT, windowEvent)    (THIS_ UINT32 message, WPARAM wParam, LPARAM lParam) PURE;

        STDMETHOD_(bool, update)            (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(EngineRegistration, "D0361A83-6C91-440B-A383-155D9A2D2FC8");
}; // namespace Gek
