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
        std::vector<IGEKRenderFilter *> m_aFilters;
        std::vector<CStringW> m_aData;
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
        float2 m_nCameraFieldOfView;
        float m_nCameraMinDistance;
        float m_nCameraMaxDistance;
        float3 m_nCameraPosition;
        float m_nBuffer;
        float4x4 m_nViewMatrix;
        float4x4 m_nProjectionMatrix;
        float4x4 m_nInvProjectionMatrix;
        float4x4 m_nTransformMatrix;
    };

    struct MATERIALBUFFER
    {
        float4 m_nColor;
        UINT32 m_bFullBright;
        float3 m_nPadding;
    };

private:
    IGEKSystem *m_pSystem;
    IGEK3DVideoSystem *m_pVideoSystem;
    IGEKEngine *m_pEngine;
    IGEKSceneManager *m_pSceneManager;

    CComPtr<IUnknown> m_spFrameEvent;

    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IUnknown> m_spPixelProgram;
    CComPtr<IGEK3DVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spIndexBuffer;

    CComPtr<IUnknown> m_spPointSampler;
    CComPtr<IUnknown> m_spLinearSampler;

    CComPtr<IGEK3DVideoBuffer> m_spOrthoBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spEngineBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spMaterialBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spLightCountBuffer;
    CComPtr<IGEK3DVideoBuffer> m_spLightBuffer;
    CComPtr<IGEK3DVideoTexture> m_spScreenBuffer[2];
    UINT8 m_nCurrentScreenBuffer;
    CComPtr<IUnknown> m_spBlendStates;
    CComPtr<IUnknown> m_spRenderStates;
    CComPtr<IUnknown> m_spDepthStates;
    UINT32 m_nNumLightInstances;

    std::unordered_map<CStringW, CComPtr<IUnknown>> m_aResources;
    std::unordered_map<CStringW, CComPtr<IGEKRenderFilter>> m_aFilters;
    std::unordered_map<CStringW, PASS> m_aPasses;

    frustum m_nCurrentFrustum;
    ENGINEBUFFER m_kCurrentBuffer;
    GEK3DVIDEO::VIEWPORT m_kScreenViewPort;
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
    STDMETHOD_(bool, EnableMaterial)        (THIS_ IGEK3DVideoContext *pContext, IUnknown *pMaterial);

    // IGEKProgramManager
    STDMETHOD(LoadProgram)                  (THIS_ LPCWSTR pName, IUnknown **ppProgram);
    STDMETHOD_(void, EnableProgram)         (THIS_ IGEK3DVideoContext *pContext, IUnknown *pProgram);

    // IGEKRenderSystem
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD_(void, SetResource)           (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pTexture);
    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppBuffer);
    STDMETHOD_(void, FlipScreens)           (THIS);
    STDMETHOD_(void, SetScreenTargets)      (THIS_ IGEK3DVideoContext *pContext, IUnknown *pDepthBuffer);
    STDMETHOD_(void, DrawScene)             (THIS_ IGEK3DVideoContext *pContext, UINT32 nAttributes);
    STDMETHOD_(void, DrawLights)            (THIS_ IGEK3DVideoContext *pContext, std::function<void(void)> OnLightBatch);
    STDMETHOD_(void, DrawOverlay)           (THIS_ IGEK3DVideoContext *pContext);
    STDMETHOD_(void, Render)                (THIS);

    // IGEKRenderManager
    STDMETHOD_(float2, GetScreenSize)       (THIS) const;
    STDMETHOD_(const frustum &, GetFrustum) (THIS) const;
};