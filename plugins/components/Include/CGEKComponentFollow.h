#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentFollow : public CGEKUnknown
                          , public IGEKComponent
{
public:
    struct DATA
    {
    public:
        CStringW m_strTarget;
        float3 m_nOffset;
        quaternion m_nRotation;

    public:
        DATA(void)
        {
        }
    };

public:
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;

public:
    DECLARE_UNKNOWN(CGEKComponentFollow)
    CGEKComponentFollow(void);
    ~CGEKComponentFollow(void);

    // IGEKComponent
    STDMETHOD_(LPCWSTR, GetName)                (THIS) const;
    STDMETHOD_(void, Clear)                     (THIS);
    STDMETHOD(AddComponent)                     (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(RemoveComponent)                  (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(bool, HasComponent)              (THIS_ const GEKENTITYID &nEntityID) const;
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) const;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue);
};

class CGEKComponentSystemFollow : public CGEKUnknown
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
{
private:
    IGEKSceneManager *m_pSceneManager;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemFollow)
    CGEKComponentSystemFollow(void);
    ~CGEKComponentSystemFollow(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKSceneObserver
    STDMETHOD_(void, OnPostUpdate)          (THIS_ float nGameTime, float nFrameTime);
};