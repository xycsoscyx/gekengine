#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKLightMoveState : public CGEKUnknown
                         , public IGEKLogicState
{
private:
    IGEKEntity *m_pEntity;
    float3 m_nOrigin;
    float m_nOffset;
    float m_nSpeed;
    float m_nSize;

public:
    CGEKLightMoveState(void);
    virtual ~CGEKLightMoveState(void);
    DECLARE_UNKNOWN(CGEKLightMoveState);

    // IGEKLogicState
    STDMETHOD_(void, OnEnter)           (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(void, OnExit)            (THIS);
    STDMETHOD_(void, OnUpdate)          (THIS_ float nGameTime, float nFrameTime);
};