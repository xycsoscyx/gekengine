#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE_IID_(IGEKSceneManager, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
{
    STDMETHOD(LoadScene)                (LPCWSTR pName, LPCWSTR pEntry) PURE;

    STDMETHOD(AddEntity)                (THIS_ CLibXMLNode &kEntity) PURE;
    STDMETHOD(FindEntity)               (THIS_ LPCWSTR pName, IGEKEntity **ppEntity) PURE;
    STDMETHOD(DestroyEntity)            (THIS_ IGEKEntity *pEntity) PURE;

    STDMETHOD_(float3, GetGravity)      (THIS_ const float4 &nGravity) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSceneObserver, IGEKObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
{
    STDMETHOD(OnLoadBegin)              (THIS) { return S_OK; };
    STDMETHOD(OnStaticFace)             (THIS_ float3 *pFace, IUnknown *pMaterial) { return S_OK; };
    STDMETHOD(OnLoadEnd)                (THIS_ HRESULT hRetVal) { return S_OK; };

    STDMETHOD(OnEntityAdded)            (THIS_ IGEKEntity *pEntity) { return S_OK; };
    STDMETHOD(OnEntityDestroyed)        (THIS_ IGEKEntity *pEntity) { return S_OK; };

    STDMETHOD(OnPreUpdate)              (THIS_ float nGameTime, float nFrameTime) { return S_OK; };
    STDMETHOD(OnUpdate)                 (THIS_ float nGameTime, float nFrameTime) { return S_OK; };
    STDMETHOD(OnPostUpdate)             (THIS_ float nGameTime, float nFrameTime) { return S_OK; };
    STDMETHOD(OnRender)                 (THIS) { return S_OK; };
};

SYSTEM_USER(SceneManager, "D7455474-94FE-4282-92F1-7DD662EBC90E");