#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <list>

class CGEKComponentModel : public CGEKUnknown
                         , public CGEKComponent
                         , public CGEKModelManagerUser
                         , public CGEKViewManagerUser
{
private:
    CComPtr<IUnknown> m_spModel;
    CStringW m_strModel;
    CStringW m_strParams;
    float4 m_nMaterialParams;

public:
    DECLARE_UNKNOWN(CGEKComponentModel)
    CGEKComponentModel(IGEKEntity *pEntity);
    ~CGEKComponentModel(void);

    void OnRender(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
};

class CGEKComponentSystemModel : public CGEKUnknown
                               , public CGEKContextUser
                               , public CGEKSceneManagerUser
                               , public CGEKViewManagerUser
                               , public IGEKSceneObserver
                               , public IGEKComponentSystem
{
private:
    std::map<IGEKEntity *, CComPtr<CGEKComponentModel>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemModel)
    CGEKComponentSystemModel(void);
    ~CGEKComponentSystemModel(void);

    // IGEKUnknown
    STDMETHOD(Initialize)       (THIS);
    STDMETHOD_(void, Destroy)   (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)     (THIS);
    STDMETHOD(Destroy)          (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)           (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);

    // IGEKSceneObserver
    STDMETHOD(OnRender)         (THIS);
};