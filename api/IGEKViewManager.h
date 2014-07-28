#pragma once

#include "GEKContext.h"
#include "IGEKComponent.h"

DECLARE_INTERFACE_IID_(IGEKViewManager, IUnknown, "585D122C-2488-4EEE-9FED-A7B0A421A324")
{
    STDMETHOD(SetViewer)                (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(GEKENTITYID, GetViewer)  (THIS) const PURE;
};
