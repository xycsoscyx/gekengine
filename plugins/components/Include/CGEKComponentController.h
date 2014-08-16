#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

class CGEKComponentController : public CGEKUnknown
                              , public IGEKComponent
{
public:
    struct DATA
    {
    public:
        float m_nTurn;
        float m_nTilt;

    public:
        DATA(void)
            : m_nTurn(0.0f)
            , m_nTilt(0.0f)
        {
        }
    };

public:
    concurrency::concurrent_unordered_map<GEKENTITYID, DATA> m_aData;

public:
    DECLARE_UNKNOWN(CGEKComponentController)
    CGEKComponentController(void);
    ~CGEKComponentController(void);

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

class CGEKComponentSystemController : public CGEKUnknown
                                    , public IGEKInputObserver
                                    , public IGEKSceneObserver
                                    , public IGEKComponentSystem
{
private:
    std::unordered_map<GEKENTITYID, std::unordered_map<CStringW, float>> m_aActions;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemController)
    CGEKComponentSystemController(void);
    ~CGEKComponentSystemController(void);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKInputObserver
    STDMETHOD_(void, OnAction)              (THIS_ LPCWSTR pName, const GEKVALUE &kValue);

    // IGEKSceneObserver
    STDMETHOD_(void, OnPreUpdate)           (THIS_ float nGameTime, float nFrameTime);
};