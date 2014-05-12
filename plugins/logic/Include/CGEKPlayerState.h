#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKPlayerState : public CGEKUnknown
                      , public CGEKLogicSystemUser
                      , public IGEKLogicState
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

    // IGEKLogicState
    STDMETHOD_(void, OnEnter)           (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(void, OnExit)            (THIS);
    STDMETHOD_(void, OnEvent)           (THIS_ LPCWSTR pAction, const GEKVALUE &kParamA, const GEKVALUE &kParamB);
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime);
};