#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentModel : public CGEKUnknown
                         , public CGEKComponent
                         , public CGEKModelManagerUser
                         , public CGEKViewManagerUser
{
public:
    CStringW m_strSource;
    CStringW m_strParams;
    CComPtr<IUnknown> m_spModel;

public:
    DECLARE_UNKNOWN(CGEKComponentModel)
    CGEKComponentModel(IGEKEntity *pEntity);
    ~CGEKComponentModel(void);

    // IGEKComponent
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
    STDMETHOD(OnEntityCreated)              (THIS);
};

class CGEKComponentSystemModel : public CGEKUnknown
                               , public CGEKContextUser
                               , public CGEKSceneManagerUser
                               , public CGEKViewManagerUser
                               , public IGEKComponentSystem
{
private:
    concurrency::concurrent_unordered_map<IGEKEntity *, CComPtr<CGEKComponentModel>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemModel)
    CGEKComponentSystemModel(void);
    ~CGEKComponentSystemModel(void);

    // IGEKComponentSystem
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, Clear)                 (THIS);
    STDMETHOD(Destroy)                      (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                       (THIS_ const CLibXMLNode &kEntityNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);
    STDMETHOD_(void, GetVisible)            (THIS_ const frustum &kFrustum, concurrency::concurrent_unordered_set<IGEKEntity *> &aVisibleEntities);
};