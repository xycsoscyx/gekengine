#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKPlayerState : public CGEKUnknown
                      , public IGEKLogicState
                      , public IGEKInputObserver
{
private:
    IGEKEntity *m_pEntity;
    std::map<GEKHASH, bool> m_aActions;
    float2 m_nRotation;
    bool m_bActive;

public:
    CGEKPlayerState(void);
    virtual ~CGEKPlayerState(void);
    DECLARE_UNKNOWN(CGEKPlayerState);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);
    STDMETHOD_(void, Destroy)           (THIS);

    // IGEKLogicState
    STDMETHOD_(void, OnEnter)           (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(void, OnExit)            (THIS);
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime);

    // IGEKInputObserver
    STDMETHOD_(void, OnAction)          (THIS_ LPCWSTR pEvent, const GEKVALUE &kValue);
};