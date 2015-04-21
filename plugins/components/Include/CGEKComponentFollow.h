#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(follow, CStringW, 0x00000010)
    DECLARE_COMPONENT_VALUE(float3, offset)
    DECLARE_COMPONENT_VALUE(quaternion, rotation)
END_DECLARE_COMPONENT(follow)

class CGEKComponentSystemFollow : public CGEKUnknown
                                , public IGEKSceneObserver
                                , public IGEKComponentSystem
{
private:
    IGEKEngineCore *m_pEngine;

public:
    DECLARE_UNKNOWN(CGEKComponentSystemFollow)
    CGEKComponentSystemFollow(void);
    ~CGEKComponentSystemFollow(void);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKSceneObserver
    STDMETHOD_(void, OnUpdateEnd)               (THIS_ float nGameTime, float nFrameTime);
};