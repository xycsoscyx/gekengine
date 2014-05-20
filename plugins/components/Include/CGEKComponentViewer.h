#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentViewer : public CGEKUnknown
                          , public CGEKComponent
{
private:
    float m_nFieldOfView;
    float m_nMinViewDistance;
    float m_nMaxViewDistance;

public:
    DECLARE_UNKNOWN(CGEKComponentViewer)
    CGEKComponentViewer(IGEKContext *pContext, IGEKEntity *pEntity);
    ~CGEKComponentViewer(void);

    // IGEKComponent
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemViewer : public CGEKUnknown
                                , public IGEKComponentSystem
{
private:
    concurrency::concurrent_unordered_map<IGEKEntity *, CComPtr<CGEKComponentViewer>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemViewer)
    CGEKComponentSystemViewer(void);
    ~CGEKComponentSystemViewer(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);

    // IGEKComponentSystem
    STDMETHOD_(LPCWSTR, GetType)            (THIS) const;
    STDMETHOD_(void, Clear)                 (THIS);
    STDMETHOD(Destroy)                      (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)                       (THIS_ const CLibXMLNode &kComponentNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);
};