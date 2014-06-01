#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentLight : public CGEKUnknown
                         , public CGEKComponent
{
public:
    float3 m_nColor;
    float m_nRange;

public:
    DECLARE_UNKNOWN(CGEKComponentLight)
    CGEKComponentLight(IGEKContext *pContext, IGEKEntity *pEntity);
    ~CGEKComponentLight(void);

    // IGEKComponent
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemLight : public CGEKUnknown
                               , public IGEKSceneObserver
                               , public IGEKComponentSystem
{
private:
    concurrency::concurrent_unordered_map<IGEKEntity *, CComPtr<CGEKComponentLight>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemLight)
    CGEKComponentSystemLight(void);
    ~CGEKComponentSystemLight(void);

    // IGEKSceneObserver
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKComponentSystem
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD(Destroy)                      (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                       (THIS_ const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);
    STDMETHOD_(void, GetVisible)            (THIS_ const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities);
};