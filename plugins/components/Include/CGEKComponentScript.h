#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <lua.hpp>
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <list>

class CGEKComponentScript : public CGEKUnknown
                          , public CGEKComponent
                          , public CGEKViewManagerUser
{
private:
    CStringW m_strScript;
    CStringW m_strParams;

    lua_State *m_pState;
    luabind::object m_kCurrentState;
    bool m_bOnRender;

public:
    DECLARE_UNKNOWN(CGEKComponentScript)
    CGEKComponentScript(IGEKEntity *pEntity, lua_State *pState);
    ~CGEKComponentScript(void);

    bool HasOnRender(void);
    void SetCurrentState(const luabind::object &kState);
    luabind::object &GetCurrentState(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
    STDMETHOD_(void, OnEvent)               (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB);
};

class CGEKComponentSystemScript : public CGEKUnknown
                                , public CGEKContextUser
                                , public CGEKSceneManagerUser
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
{
private:
    std::map<IGEKEntity *, CComPtr<CGEKComponentScript>> m_aComponents;
    lua_State *m_pState;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemScript)
    CGEKComponentSystemScript(void);
    ~CGEKComponentSystemScript(void);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)             (THIS);
    STDMETHOD(Destroy)                  (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                   (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD_(void, OnPreUpdate)       (THIS_ float nGameTime, float nFrameTime);
    STDMETHOD_(void, OnRender)          (THIS_ const frustum &kFrustum);
};