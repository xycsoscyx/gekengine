#pragma once

#include "GEKUtility.h"
#include "GEKVALUE.h"

DECLARE_INTERFACE(IGEKComponent);

DECLARE_INTERFACE_IID_(IGEKEntity, IUnknown, "30A6AF4F-AD6C-4EFB-A239-11532E8B983F")
{
    STDMETHOD_(void, ListComponents)            (THIS_ std::function<void(IGEKComponent *)> OnComponent) PURE;
    STDMETHOD_(IGEKComponent *, GetComponent)   (THIS_ LPCWSTR pName) PURE;

    STDMETHOD_(void, OnEvent)                   (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA = GEKVALUE(), const GEKVALUE &kParamB = GEKVALUE()) PURE;
};
