#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKComponent);

typedef UINT32 GEKENTITYID;

DECLARE_INTERFACE_IID_(IGEKSceneManager, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
{
    STDMETHOD(Load)                             (THIS_ LPCWSTR pName, LPCWSTR pEntry) PURE;

    STDMETHOD_(float3, GetGravity)              (THIS_ const float3 &nPosition) PURE;

    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID) PURE;
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID) PURE;

    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;

    STDMETHOD_(IGEKComponent *, GetComponent)   (THIS_ LPCWSTR pComponent) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSceneObserver, IGEKObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
{
    STDMETHOD_(void, OnLoadBegin)       (THIS) { };
    STDMETHOD(OnLoadEnd)                (THIS_ HRESULT hRetVal) { return S_OK; };

    STDMETHOD_(void, OnFree)            (THIS) { };

    STDMETHOD_(void, OnPreUpdate)       (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnPostUpdate)      (THIS_ float nGameTime, float nFrameTime) { };
};
