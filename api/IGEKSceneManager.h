#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKSceneManager, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
{
    STDMETHOD(LoadScene)                (LPCWSTR pName, LPCWSTR pEntry) PURE;

    STDMETHOD(AddEntity)                (THIS_ CLibXMLNode &kEntityNode) PURE;
    STDMETHOD(FindEntity)               (THIS_ LPCWSTR pName, IGEKEntity **ppEntity) PURE;
    STDMETHOD(DestroyEntity)            (THIS_ IGEKEntity *pEntity) PURE;

    STDMETHOD_(float3, GetGravity)      (THIS_ const float4 &nGravity) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSceneObserver, IGEKObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
{
    STDMETHOD_(void, OnLoadBegin)       (THIS) { };
    STDMETHOD(OnLoadEnd)                (THIS_ HRESULT hRetVal) { return S_OK; };

    STDMETHOD_(void, OnEntityAdded)     (THIS_ IGEKEntity *pEntity) { };
    STDMETHOD_(void, OnEntityDestroyed) (THIS_ IGEKEntity *pEntity) { };

    STDMETHOD_(void, OnPreUpdate)       (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnPostUpdate)      (THIS_ float nGameTime, float nFrameTime) { };
};

SYSTEM_USER(SceneManager, "D7455474-94FE-4282-92F1-7DD662EBC90E");