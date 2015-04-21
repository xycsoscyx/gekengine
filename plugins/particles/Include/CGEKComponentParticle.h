#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

class CGEKComponentSystemParticle : public CGEKUnknown
                                  , public IGEKComponentSystem
                                  , public IGEKRenderObserver
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

    CComPtr<IGEK3DVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;

    CComPtr<IGEK3DVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spIndexBuffer;

    std::map<std::pair<IUnknown *, IGEK3DVideoTexture *>, std::vector<INSTANCE>> m_aVisible;

public:
    CGEKComponentSystemParticle(void);
    virtual ~CGEKComponentSystemParticle(void);
    DECLARE_UNKNOWN(CGEKComponentSystemParticle);

    // IGEKComponentSystem
    STDMETHOD(Initialize)                       (THIS_ IGEKEngineCore *pEngine);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS_ const GEKENTITYID &nViewerID);
    STDMETHOD_(void, OnCullScene)               (THIS_ const GEKENTITYID &nViewerID, const frustum &nViewFrustum);
    STDMETHOD_(void, OnDrawScene)               (THIS_ const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS_ const GEKENTITYID &nViewerID);
};