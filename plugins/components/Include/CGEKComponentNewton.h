#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <Newton.h>
#include <concurrent_unordered_map.h>

DECLARE_INTERFACE_IID_(IGEKNewtonSystem, IUnknown, "46D22819-49E1-4CB3-9B9A-41663F1DE553")
{
    STDMETHOD_(NewtonWorld *, GetWorld)             (THIS) PURE;
    STDMETHOD_(NewtonCollision *, LoadCollision)    (THIS_ LPCWSTR pShape, LPCWSTR pParams) PURE;
};

class CGEKComponentNewton : public CGEKUnknown
                          , public CGEKComponent
{
public:
    NewtonBody *m_pBody;
    CStringW m_strShape;
    CStringW m_strParams;
    float m_nMass;

public:
    DECLARE_UNKNOWN(CGEKComponentNewton)
    CGEKComponentNewton(IGEKContext *pContext, IGEKEntity *pEntity);
    ~CGEKComponentNewton(void);

    // IGEKComponent
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
};

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
                                , public IGEKNewtonSystem
{
private:
    NewtonWorld *m_pWorld;
    concurrency::concurrent_unordered_map<IGEKEntity *, CComPtr<CGEKComponentNewton>> m_aComponents;
    std::map<GEKHASH, NewtonCollision *> m_aCollisions;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                           (THIS);
    STDMETHOD_(void, Destroy)                       (THIS);

    // IGEKComponentSystem
    STDMETHOD_(LPCWSTR, GetType)                    (THIS) const;
    STDMETHOD_(void, Clear)                         (THIS);
    STDMETHOD(Destroy)                              (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                               (THIS_ const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD_(void, OnLoadBegin)                   (THIS);
    STDMETHOD(OnLoadEnd)                            (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnUpdate)                      (THIS_ float nGameTime, float nFrameTime);

    // IGEKNewtonSystem
    STDMETHOD_(NewtonWorld *, GetWorld)             (THIS);
    STDMETHOD_(NewtonCollision *, LoadCollision)    (THIS_ LPCWSTR pShape, LPCWSTR pParams);
};