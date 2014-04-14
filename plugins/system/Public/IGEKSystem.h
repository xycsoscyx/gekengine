#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKSystem, IUnknown, "5BA2B424-8708-4703-A731-B585F4F43716")
{
    STDMETHOD(Reset)                                (THIS) PURE;

    STDMETHOD_(CGEKConfig &, GetConfig)             (THIS) PURE;
    STDMETHOD_(HWND, GetWindow)                     (THIS) PURE;
    STDMETHOD_(bool, IsWindowed)                    (THIS) PURE;
    STDMETHOD_(UINT32, GetXSize)                    (THIS) PURE;
    STDMETHOD_(UINT32, GetYSize)                    (THIS) PURE;
    STDMETHOD_(UINT32, ParseValue)                  (THIS_ LPCWSTR pValue) PURE;

    STDMETHOD_(bool, IsRunning)                     (THIS) PURE;
    STDMETHOD_(void, Run)                           (THIS) PURE;
    STDMETHOD_(void, Stop)                          (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSystemObserver, IGEKObserver, "61B9B274-F0CA-4F9B-A284-48790FC3AB81")
{
    STDMETHOD(OnEvent)                              (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult) { return S_OK; };
    STDMETHOD(OnRun)                                (THIS) { return S_OK; };
    STDMETHOD(OnStop)                               (THIS) { return S_OK; };
    STDMETHOD(OnStep)                               (THIS) { return S_OK; };
};

SYSTEM_USER(System, "71F36891-7A82-4A07-9471-FAEB94E37B03");
