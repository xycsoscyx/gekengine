#pragma once

#include "GEKUtility.h"
#include "GEKVALUE.h"

DECLARE_INTERFACE(IGEKEntity);

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const PURE;
    STDMETHOD_(IGEKEntity *, GetEntity)     (THIS) const PURE;

    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) PURE;
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const PURE;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue) PURE;

    STDMETHOD(OnEntityCreated)              (THIS) { return S_OK; }
    STDMETHOD_(void, OnEntityDestroyed)     (THIS) { }
    STDMETHOD_(void, OnEvent)               (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB) { };
};

