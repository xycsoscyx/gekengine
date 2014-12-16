#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEKComponent);

typedef UINT32 GEKENTITYID;
const GEKENTITYID GEKINVALIDENTITYID = 0;

DECLARE_INTERFACE_IID_(IGEKSceneManager, IUnknown, "43DF2FD7-3BE2-4333-86ED-CB1221C6599B")
{
private:
    STDMETHOD_(LPVOID, GetComponent)            (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;

public:
    STDMETHOD(Load)                             (THIS_ LPCWSTR pName) PURE;

    STDMETHOD(CreateEntity)                     (THIS_ GEKENTITYID &nEntityID, LPCWSTR pName = nullptr) PURE;
    STDMETHOD(DestroyEntity)                    (THIS_ const GEKENTITYID &nEntityID) PURE;
    STDMETHOD(GetNamedEntity)                   (THIS_ LPCWSTR pName, GEKENTITYID *pEntityID) PURE;

    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent, const std::unordered_map<CStringW, CStringW> &aParams) PURE;
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) PURE;

    template <typename CLASS>
    CLASS &GetComponent(const GEKENTITYID &nEntityID, LPCWSTR pComponent)
    {
        return *(CLASS *)GetComponent(nEntityID, pComponent);
    }

    STDMETHOD_(void, ListEntities)              (THIS_ std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false) PURE;
    STDMETHOD_(void, ListComponentsEntities)    (THIS_ const std::vector<CStringW> &aComponents, std::function<void(const GEKENTITYID &)> OnEntity, bool bParallel = false) PURE;

    STDMETHOD_(float3, GetGravity)              (THIS_ const float3 &nPosition) const PURE;
};

DECLARE_INTERFACE_IID_(IGEKSceneObserver, IGEKObserver, "51D6E5E6-2AD3-4D61-A704-8E6515F024F9")
{
    STDMETHOD_(void, OnLoadBegin)               (THIS) { };
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal) { return S_OK; };

    STDMETHOD_(void, OnFree)                    (THIS) { };

    STDMETHOD_(void, OnEntityCreated)           (THIS_ const GEKENTITYID &nEntityID) { };
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID) { };
    STDMETHOD_(void, OnComponentAdded)          (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) { };
    STDMETHOD_(void, OnComponentRemoved)        (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent) { };

    STDMETHOD_(void, OnPreUpdate)               (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime) { };
    STDMETHOD_(void, OnPostUpdate)              (THIS_ float nGameTime, float nFrameTime) { };
};
