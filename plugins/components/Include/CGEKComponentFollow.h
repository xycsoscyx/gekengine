#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"
#include <concurrent_unordered_map.h>

DECLARE_COMPONENT(follow)
    DECLARE_COMPONENT_VALUE(CStringW, target)
    DECLARE_COMPONENT_VALUE(float3, offset)
    DECLARE_COMPONENT_VALUE(quaternion, rotation)
END_DECLARE_COMPONENT(follow)

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
    STDMETHOD_(void, OnUpdateEnd)           (THIS_ float nGameTime, float nFrameTime);
};