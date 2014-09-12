#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKRenderSystem.h"
#include "IGEKPopulationSystem.h"
#include "IGEKEngine.h"
#include <concurrent_vector.h>

DECLARE_INTERFACE(IGEKRenderFilter);
DECLARE_INTERFACE(IGEKMaterial);

class CGEKRenderSystem : public CGEKUnknown
                       , public CGEKObservable
                       , public IGEKSceneObserver
                       , public IGEKRenderSystem
                       , public IGEKProgramManager
                       , public IGEKMaterialManager
                       , public IGEKRenderManager
{
public:
    struct PASS
    {
        std::vector<PASS *> m_aRequiredPasses;
        std::vector<IGEKRenderFilter *> m_aFilters;
    };

    struct LIGHT
    {
        float3 m_nPosition;
        float m_nRange;
        float3 m_nColor;
        float m_nInvRange;
    };

    struct ENGINEBUFFER
    {
        float2 m_nCameraSize;
        float2 m_nCameraView;
        float m_nCameraViewDistance;
        float3 m_nCameraPosition;
        float4x4 m_nViewMatrix;
        float4x4 m_nProjectionMatrix;
        float4x4 m_nTransformMatrix;
    };

private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;
    IGEKEngine *m_pEngine;
    IGEKSceneManager *m_pSceneManager;

    CComPtr<IUnknown> m_spFrameEvent;

    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IUnknown> m_spPixelProgram;
    CComPtr<IGEKVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEKVideoBuffer> m_spIndexBuffer;

    CComPtr<IUnknown> m_spPointSampler;
    CComPtr<IUnknown> m_spLinearSampler;

    CComPtr<IGEKVideoBuffer> m_spOrthoBuffer;
    CComPtr<IGEKVideoBuffer> m_spEngineBuffer;
    CComPtr<IGEKVideoBuffer> m_spLightCountBuffer;
    CComPtr<IGEKVideoBuffer> m_spLightBuffer;
    CComPtr<IGEKVideoTexture> m_spScreenBuffer;
    CComPtr<IUnknown> m_spBlendStates;
    CComPtr<IUnknown> m_spRenderStates;
    CComPtr<IUnknown> m_spDepthStates;
    UINT32 m_nNumLightInstances;

    std::unordered_map<CStringW, CComPtr<IUnknown>> m_aResources;
    std::unordered_map<CStringW, CComPtr<IGEKRenderFilter>> m_aFilters;
    std::unordered_map<CStringW, PASS> m_aPasses;

    frustum m_nCurrentFrustum;
    ENGINEBUFFER m_kCurrentBuffer;
    GEKVIDEO::VIEWPORT m_kScreenViewPort;
    std::vector<LIGHT> m_aVisibleLights;

    PASS *m_pCurrentPass;
    IGEKRenderFilter *m_pCurrentFilter;

private:
    HRESULT LoadPass(LPCWSTR pName);

public:
    CGEKRenderSystem(void);
    virtual ~CGEKRenderSystem(void);
    DECLARE_UNKNOWN(CGEKRenderSystem);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKSceneObserver
    STDMETHOD_(void, OnLoadBegin)           (THIS);
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKMaterialManager
    STDMETHOD(LoadMaterial)                 (THIS_ LPCWSTR pName, IUnknown **ppMaterial);
    STDMETHOD_(bool, EnableMaterial)        (THIS_ IUnknown *pMaterial);

    // IGEKProgramManager
    STDMETHOD(LoadProgram)                  (THIS_ LPCWSTR pName, IUnknown **ppProgram);
    STDMETHOD_(void, EnableProgram)         (THIS_ IUnknown *pProgram);

    // IGEKRenderSystem
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pTexture);
    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppBuffer);
    STDMETHOD_(void, SetScreenTargets)      (THIS_ IUnknown *pDepthBuffer);
    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes);
    STDMETHOD_(void, DrawLights)            (THIS_ std::function<void(void)> OnLightBatch);
    STDMETHOD_(void, DrawOverlay)           (THIS);
    STDMETHOD_(void, Render)                (THIS);

    // IGEKRenderManager
    STDMETHOD_(float2, GetScreenSize)       (THIS) const;
    STDMETHOD_(const frustum &, GetFrustum) (THIS) const;
};