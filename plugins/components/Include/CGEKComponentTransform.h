#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentTransform : public CGEKUnknown
                             , public CGEKComponent
{
private:
    float3 m_nPosition;
    quaternion m_nRotation;

public:
    DECLARE_UNKNOWN(CGEKComponentTransform)
    CGEKComponentTransform(IGEKContext *pContext, IGEKEntity *pEntity);
    ~CGEKComponentTransform(void);

    // IGEKComponent
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemTransform : public CGEKUnknown
                                   , public IGEKSceneObserver
                                   , public IGEKComponentSystem
{
private:
    concurrency::concurrent_unordered_map<IGEKEntity *, CComPtr<CGEKComponentTransform>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemTransform)
    CGEKComponentSystemTransform(void);
    ~CGEKComponentSystemTransform(void);

    // IGEKSceneObserver
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKComponentSystem
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD(Destroy)                      (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                       (THIS_ const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);
};