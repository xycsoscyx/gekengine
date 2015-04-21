#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

class CGEKComponentSystemOffset : public CGEKUnknown
                                , public IGEKComponentSystem
                                , public IGEKSceneObserver
{
public:
    struct INSTANCE
    {
        float3 m_nPosition;
        float m_nDistance;
        float m_nAge;
        float m_nSpin;
        float m_nSize;
        float4 m_nColor;

        INSTANCE(const float3 &nPosition, float nDistance, float nAge, float nSize, float nSpin, const float4 &nColor)
            : m_nPosition(nPosition)
            , m_nDistance(nDistance)
            , m_nAge(nAge)
            , m_nSize(nSize)
            , m_nSpin(nSpin)
            , m_nColor(nColor)
        {
        }
    };

private:
    IGEKEngineCore *m_pEngine;

public:
    CGEKComponentSystemOffset(void);
    virtual ~CGEKComponentSystemOffset(void);
    DECLARE_UNKNOWN(CGEKComponentSystemOffset);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);
    STDMETHOD_(void, OnEntityCreated)           (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);
};