#pragma once

#include "GEKContext.h"
#include "IGEKComponent.h"

DECLARE_INTERFACE_IID_(IGEKViewManager, IUnknown, "585D122C-2488-4EEE-9FED-A7B0A421A324")
{
    STDMETHOD(SetViewer)                (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(GEKENTITYID, GetViewer)  (THIS) const PURE;

    STDMETHOD(ShowLight)                (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD(ShowModel)                (THIS_ const GEKENTITYID &nEntityID) PURE;
};

DECLARE_INTERFACE_IID_(IGEKViewObserver, IGEKObserver, "91E2AD0F-2C01-4F9F-A345-72B4469E9949")
{
    STDMETHOD_(void, OnRender)          (THIS) { };
};
