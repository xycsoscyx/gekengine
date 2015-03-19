#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

DECLARE_COMPONENT(flames, 0x00000102)
    DECLARE_COMPONENT_VALUE(CStringW, material)
    DECLARE_COMPONENT_VALUE(CStringW, gradient)
    DECLARE_COMPONENT_VALUE(float4, color)
    DECLARE_COMPONENT_VALUE(float4, color_offset)
    DECLARE_COMPONENT_VALUE(UINT32, density)
    DECLARE_COMPONENT_VALUE(float2, life_range)
    DECLARE_COMPONENT_VALUE(float3, direction)
    DECLARE_COMPONENT_VALUE(float3, direction_offset)
    DECLARE_COMPONENT_VALUE(float3, position_offset)
    DECLARE_COMPONENT_VALUE(float2, spin_range)
    DECLARE_COMPONENT_VALUE(float2, size_min_range)
    DECLARE_COMPONENT_VALUE(float2, size_max_range)
    DECLARE_COMPONENT_VALUE(float2, mass_range)
END_DECLARE_COMPONENT(flames)

class CGEKComponentSystemFlames : public CGEKUnknown
                                , public IGEKComponentSystem
                                , public IGEKSceneObserver
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

    struct PARTICLE
    {
        float2 m_nLife;
        float3 m_nPosition;
        float3 m_nVelocity;
        float2 m_nSpin;
        float2 m_nSize;
        float m_nMass;
        float4 m_nColor;
    };

    struct EMITTER : public aabb
    {
        std::vector<PARTICLE> m_aParticles;
    };

private:
    CComPtr<IGEK3DVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;
    IGEKSceneManager *m_pSceneManager;
    IGEKRenderManager *m_pRenderManager;
    IGEK3DVideoSystem *m_pVideoSystem;
    IGEKMaterialManager *m_pMaterialManager;
    IGEKProgramManager *m_pProgramManager;

    CComPtr<IGEK3DVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spIndexBuffer;

    concurrency::concurrent_unordered_map<GEKENTITYID, EMITTER> m_aEmitters;
    std::map<std::pair<IUnknown *, IGEK3DVideoTexture *>, std::vector<INSTANCE>> m_aVisible;

public:
    CGEKComponentSystemFlames(void);
    virtual ~CGEKComponentSystemFlames(void);
    DECLARE_UNKNOWN(CGEKComponentSystemFlames);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                        (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                    (THIS);
    STDMETHOD_(void, OnEntityDestroyed)         (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(void, OnComponentAdded)          (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(void, OnComponentRemoved)        (THIS_ const GEKENTITYID &nEntityID, const GEKCOMPONENTID &nComponentID);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS_ const GEKENTITYID &nViewerID);
    STDMETHOD_(void, OnCullScene)               (THIS_ const GEKENTITYID &nViewerID, const frustum &nViewFrustum);
    STDMETHOD_(void, OnDrawScene)               (THIS_ const GEKENTITYID &nViewerID, IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS_ const GEKENTITYID &nViewerID);
};