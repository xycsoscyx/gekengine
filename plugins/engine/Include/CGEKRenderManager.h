#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKRenderManager.h"
#include "IGEKPopulationManager.h"
#include "IGEKEngine.h"
#include <concurrent_vector.h>

DECLARE_INTERFACE(IGEKRenderFilter);
DECLARE_INTERFACE(IGEKMaterial);

class CGEKRenderManager : public CGEKUnknown
                        , public CGEKObservable
                        , public IGEKSceneObserver
                        , public IGEKRenderManager
                        , public IGEKProgramManager
                        , public IGEKMaterialManager
                        , public IGEKViewManager
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

    CComPtr<IUnknown> m_spModelManager;

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

    std::map<CStringW, CComPtr<IUnknown>> m_aResources;
    std::map<CStringW, CComPtr<IGEKRenderFilter>> m_aFilters;
    std::map<CStringW, PASS> m_aPasses;

    GEKENTITYID m_nViewerEntityID;
    frustum m_kFrustum;
    ENGINEBUFFER m_kEngineBuffer;

    PASS *m_pCurrentPass;
    IGEKRenderFilter *m_pCurrentFilter;
    std::map<PASS *, INT32> m_aCurrentPasses;

    concurrency::concurrent_vector<GEKENTITYID> m_aShownModels;
    std::map<IGEKModel *, std::vector<IGEKModel::INSTANCE>> m_aVisibleModels;

    concurrency::concurrent_vector<GEKENTITYID> m_aShownLights;
    std::vector<LIGHT> m_aVisibleLights;

private:
    HRESULT LoadPass(LPCWSTR pName);

public:
    CGEKRenderManager(void);
    virtual ~CGEKRenderManager(void);
    DECLARE_UNKNOWN(CGEKRenderManager);

    // IGEKSceneObserver
    STDMETHOD_(void, OnLoadBegin)           (THIS);
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, OnFree)                (THIS);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKMaterialManager
    STDMETHOD(LoadMaterial)                 (THIS_ LPCWSTR pName, IUnknown **ppMaterial);
    STDMETHOD(PrepareMaterial)              (THIS_ IUnknown *pMaterial);
    STDMETHOD_(bool, EnableMaterial)        (THIS_ IUnknown *pMaterial);

    // IGEKProgramManager
    STDMETHOD(LoadProgram)                  (THIS_ LPCWSTR pName, IUnknown **ppProgram);
    STDMETHOD_(void, EnableProgram)         (THIS_ IUnknown *pProgram);

    // IGEKViewManager
    STDMETHOD(SetViewer)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD_(GEKENTITYID, GetViewer)      (THIS) const;
    STDMETHOD(ShowLight)                    (THIS_ const GEKENTITYID &nEntityID);
    STDMETHOD(ShowModel)                    (THIS_ const GEKENTITYID &nEntityID);

    // IGEKRenderManager
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pTexture);
    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppBuffer);
    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes);
    STDMETHOD_(void, DrawLights)            (THIS_ std::function<void(void)> OnLightBatch);
    STDMETHOD_(void, DrawOverlay)           (THIS);
    STDMETHOD_(void, Render)                (THIS_ bool bUpdateScreen);
};