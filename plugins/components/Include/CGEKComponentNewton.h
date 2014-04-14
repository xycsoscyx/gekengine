#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <Newton.h>
#include <list>

class CGEKComponentNewton : public CGEKUnknown
                          , public CGEKComponent
                          , public CGEKSceneManagerUser
{
private:
    NewtonWorld *m_pWorld;
    NewtonBody *m_pBody;
    CStringW m_strShape;
    CStringW m_strParams;
    float m_nMass;

public:
    DECLARE_UNKNOWN(CGEKComponentNewton)
    CGEKComponentNewton(IGEKEntity *pEntity, NewtonWorld *pWorld);
    ~CGEKComponentNewton(void);

    float GetMass(void) const;
    NewtonBody *GetBody(void) const;

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)    (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
};

class CGEKComponentSystemNewton : public CGEKUnknown
                                , public CGEKContextUser
                                , public CGEKSceneManagerUser
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
{
private:
    NewtonWorld *m_pWorld;
    std::map<IGEKEntity *, CComPtr<CGEKComponentNewton>> m_aComponents;
    NewtonCollision *m_pStatic;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemNewton)
    CGEKComponentSystemNewton(void);
    ~CGEKComponentSystemNewton(void);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)             (THIS);
    STDMETHOD(Destroy)                  (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                   (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD(OnLoadBegin)              (THIS_ void);
    STDMETHOD(OnStaticFace)             (THIS_ float3 *pFace, IUnknown *pMaterial);
    STDMETHOD(OnLoadEnd)                (THIS_ HRESULT hRetVal);
    STDMETHOD(OnUpdate)                 (THIS_ float nGameTime, float nFrameTime);
};