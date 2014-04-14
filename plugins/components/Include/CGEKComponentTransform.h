#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "CGEKComponent.h"
#include "GEKAPI.h"
#include <list>

class CGEKComponentTransform : public CGEKUnknown
                             , public CGEKComponent
{
private:
    float3 m_nPosition;
    quaternion m_nRotation;

public:
    DECLARE_UNKNOWN(CGEKComponentTransform)
    CGEKComponentTransform(IGEKEntity *pEntity);
    ~CGEKComponentTransform(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetType)    (THIS) const;
    STDMETHOD_(void, ListProperties)        (THIS_ std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty);
    STDMETHOD_(bool, GetProperty)           (THIS_ LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)           (THIS_ LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemTransform : public CGEKUnknown
                                   , public CGEKContextUser
                                   , public IGEKComponentSystem
{
private:
    std::map<IGEKEntity *, CComPtr<CGEKComponentTransform>> m_aComponents;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemTransform)
    CGEKComponentSystemTransform(void);
    ~CGEKComponentSystemTransform(void);

    // IGEKUnknown
    STDMETHOD(Initialize)       (THIS);

    // IGEKComponentSystem
    STDMETHOD_(void, Clear)     (THIS);
    STDMETHOD(Destroy)          (THIS_ IGEKEntity *pEntity);
    STDMETHOD(Create)           (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent);
};