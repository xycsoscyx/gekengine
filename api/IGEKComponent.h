#pragma once

#include "GEKUtility.h"
#include "GEKVALUE.h"
#include <concurrent_unordered_set.h>

DECLARE_INTERFACE(IGEKEntity);

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
    STDMETHOD_(IGEKEntity *, GetEntity)         (THIS) const PURE;

    STDMETHOD_(void, ListProperties)            (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) PURE;
    STDMETHOD_(bool, GetProperty)               (THIS_ LPCWSTR pName, GEKVALUE &kValue) const PURE;
    STDMETHOD_(bool, SetProperty)               (THIS_ LPCWSTR pName, const GEKVALUE &kValue) PURE;

    STDMETHOD(OnEntityCreated)                  (THIS) { return S_OK; }
    STDMETHOD_(void, OnEntityDestroyed)         (THIS) { }
    STDMETHOD_(void, OnEvent)                   (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB) { };
};

DECLARE_INTERFACE_IID_(IGEKEntity, IUnknown, "30A6AF4F-AD6C-4EFB-A239-11532E8B983F")
{
    STDMETHOD_(void, ListComponents)            (THIS_ std::function<void(IGEKComponent *)> OnComponent) PURE;
    STDMETHOD_(IGEKComponent *, GetComponent)   (THIS_ LPCWSTR pName) PURE;

    STDMETHOD_(void, OnEvent)                   (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA = GEKVALUE(), const GEKVALUE &kParamB = GEKVALUE()) PURE;
};

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
    STDMETHOD_(LPCWSTR, GetType)                (THIS) const PURE;

    STDMETHOD(Create)                           (THIS_ const CLibXMLNode &kEntityNode, IGEKEntity *pEntity, IGEKComponent **ppComponent) PURE;
    STDMETHOD(Destroy)                          (THIS_ IGEKEntity *pEntity) PURE;

    STDMETHOD_(void, GetVisible)                (THIS_ const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities) { };
};

