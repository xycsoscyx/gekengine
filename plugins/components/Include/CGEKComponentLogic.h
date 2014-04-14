#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <list>

class CGEKComponentLogic : public CGEKUnknown
                         , public CGEKComponent
{
public:
    struct STATE
    {
        std::function<void(IGEKEntity *)> OnEnter;
        std::function<void(IGEKEntity *)> OnExit;
        std::function<void(IGEKEntity *, float, float)> OnUpdate;
        std::function<void(IGEKEntity *, LPCWSTR, const GEKVALUE &, const GEKVALUE &)> OnEvent;
    };

private:
    CStringW m_strModule;
    CStringW m_strFunction;

    HMODULE m_hModule;
    STATE m_kCurrentState;

public:
    DECLARE_UNKNOWN(CGEKComponentLogic)
    CGEKComponentLogic(IGEKEntity *pEntity);
    ~CGEKComponentLogic(void);

    STATE &GetCurrentState(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
    STDMETHOD(OnEvent)                      (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB);
};

class CGEKComponentSystemLogic : public CGEKUnknown
                               , public CGEKContextUser
                               , public CGEKSceneManagerUser
                               , public IGEKSceneObserver
                               , public IGEKComponentSystem
{
private:
    std::map<IGEKEntity *, CComPtr<CGEKComponentLogic>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemLogic)
    CGEKComponentSystemLogic(void);
    ~CGEKComponentSystemLogic(void);

    // IGEKUnknown
    STDMETHOD(Initialize)       (THIS);
    STDMETHOD_(void, Destroy)   (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)     (THIS);
    STDMETHOD(Destroy)          (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)           (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD(OnPreUpdate)      (THIS_ float nGameTime, float nFrameTime);
};