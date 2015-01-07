#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include <concurrent_vector.h>

DECLARE_COMPONENT(flames)
    DECLARE_COMPONENT_VALUE(CStringW, material)
    DECLARE_COMPONENT_VALUE(CStringW, gradient)
    DECLARE_COMPONENT_VALUE(UINT32, density)
    DECLARE_COMPONENT_VALUE(float2, life)
    DECLARE_COMPONENT_VALUE(float3, direction)
    DECLARE_COMPONENT_VALUE(float3, angle)
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
        float m_nAge;

        INSTANCE(const float3 &nPosition, float nAge)
            : m_nPosition(nPosition)
            , m_nAge(nAge)
        {
        }
    };

    struct PARTICLE
    {
        float2 m_nLife;
        float3 m_nPosition;
        float3 m_nVelocity;
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
    
    std::unordered_map<CStringW, CComPtr<IGEK3DVideoTexture>> m_aGradients;

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
    STDMETHOD_(void, OnComponentAdded)          (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, OnComponentRemoved)        (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pComponent);
    STDMETHOD_(void, OnUpdate)                  (THIS_ float nGameTime, float nFrameTime);

    // IGEKRenderObserver
    STDMETHOD_(void, OnRenderBegin)             (THIS);
    STDMETHOD_(void, OnCullScene)               (THIS_ const frustum &nViewFrustum);
    STDMETHOD_(void, OnDrawScene)               (THIS_ IGEK3DVideoContext *pContext, UINT32 nVertexAttributes);
    STDMETHOD_(void, OnRenderEnd)               (THIS);
};