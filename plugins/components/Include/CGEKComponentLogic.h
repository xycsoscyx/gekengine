#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <list>

DECLARE_INTERFACE_IID_(IGEKLogicState, IUnknown, "8C0118E0-D37E-4EC6-B1BE-D036AB33F757")
{
    STDMETHOD_(void, OnEnter)           (THIS) PURE;
    STDMETHOD_(void, OnExit)            (THIS) PURE;
    STDMETHOD_(void, OnEvent)           (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB) PURE;
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime) PURE;
};

DECLARE_INTERFACE_IID_(IGEKLogicSystem, IUnknown, "CAE93234-6A56-42BA-AF8D-8A34A84B4F5C")
{
    STDMETHOD_(void, SetState)          (IGEKEntity *pEntity, IGEKLogicState *pState) PURE;
};

class CGEKComponentLogic : public CGEKUnknown
                         , public CGEKComponent
{
private:
    IGEKLogicSystem *m_pSystem;

    CStringW m_strModule;
    CStringW m_strFunction;

    HMODULE m_hModule;
    CComPtr<IGEKLogicState> m_spState;


public:
    DECLARE_UNKNOWN(CGEKComponentLogic)
    CGEKComponentLogic(IGEKLogicSystem *pSystem, IGEKEntity *pEntity);
    ~CGEKComponentLogic(void);

    void SetState(IGEKLogicState *pState);
    void OnUpdate(float nGameTime, float nFrameTime);

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

    // IGEKLogicSystem
    STDMETHOD_(void, SetState)          (IGEKEntity *pEntity, IGEKLogicState *pState);
};