#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <list>

DECLARE_INTERFACE(IGEKLogicSystem)
{
};

class CGEKComponentLogic : public CGEKUnknown
                         , public CGEKComponent
{
private:
    IGEKLogicSystem *m_pSystem;

    CStringW m_strModule;
    CStringW m_strFunction;

    HMODULE m_hModule;
    std::function<void(void *)> OnLogicDestroyed;
    std::function<void(void *, LPCWSTR, const GEKVALUE &, const GEKVALUE &)> OnLogicEvent;
    std::function<void(void *, float, float)> OnLogicUpdate;
    void *m_pData;


public:
    DECLARE_UNKNOWN(CGEKComponentLogic)
    CGEKComponentLogic(IGEKLogicSystem *pSystem, IGEKEntity *pEntity);
    ~CGEKComponentLogic(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
    STDMETHOD_(void, OnEvent)               (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB);
};

class CGEKComponentSystemLogic : public CGEKUnknown
                               , public CGEKContextUser
                               , public CGEKSceneManagerUser
                               , public IGEKSceneObserver
                               , public IGEKComponentSystem
                               , public IGEKLogicSystem
{
private:
    std::map<IGEKEntity *, CComPtr<CGEKComponentLogic>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemLogic)
    CGEKComponentSystemLogic(void);
    ~CGEKComponentSystemLogic(void);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)             (THIS);
    STDMETHOD(Destroy)                  (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                   (THIS_ const CLibXMLNode &kEntityNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD_(void, OnPreUpdate)       (THIS_ float nGameTime, float nFrameTime);
};