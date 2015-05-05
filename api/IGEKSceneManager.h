#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKComponent);

typedef UINT32 GEKENTITYID;
const GEKENTITYID GEKINVALIDENTITYID = 0;

typedef UINT32 GEKCOMPONENTID;
const GEKCOMPONENTID GEKINVALIDCOMPONENTID = 0;

DECLARE_INTERFACE_IID_(IGEKSceneManager, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
{
    STDMETHOD(Load)                             (THIS_ LPCWSTR pName) PURE;

    STDMETHOD(GetComponentSystem)               (THIS_ REFCLSID nCLSID, REFIID nIID, LPVOID FAR *ppObject) PURE;

    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID, const std::unordered_map<CStringW, std::unordered_map<CStringW, CStringW>> &aEntity, LPCWSTR pName = nullptr) PURE;
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD_(GEKENTITYID, GetNamedEntity)     (THIS_ LPCWSTR pName) PURE;

    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID) PURE;
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID) PURE;

    template <typename CLASS>
    CLASS &GetComponent(const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID)
    {
        return *(CLASS *)GetComponent(nEntityID, nComponentID);
    }

    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false) PURE;
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<GEKCOMPONENTID> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSceneObserver, IGEKObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
{
    STDMETHOD_(void, OnLoadBegin)               (THIS) { };
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal) { return S_OK; };

    STDMETHOD_(void, OnFree)                    (THIS) { };

    STDMETHOD_(void, OnEntityCreated)           (THIS_ const GEKENTITYID &nEntityID) { };
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID) { };

    STDMETHOD_(void, OnUpdateBegin)             (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnUpdateEnd)               (THIS_ float nGameTime, float nFrameTime) { };
};
