#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKRenderManager.h"
#include "IGEKPopulationManager.h"
#include "IGEKEngine.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <memory>
#include <list>

DECLARE_INTERFACE(IGEKRenderFilter);
DECLARE_INTERFACE(IGEKMaterial);

class CGEKRenderManager : public CGEKUnknown
                        , public IGEKSceneObserver
                        , public IGEKRenderManager
                        , public IGEKProgramManager
                        , public IGEKMaterialManager
                        , public IGEKModelManager
                        , public IGEKViewManager
                        , public Awesomium::DataSource
                        , public Awesomium::JSMethodHandler
                        , public Awesomium::SurfaceFactory
                        , public Awesomium::WebViewListener::View
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

    std::list<CComPtr<IGEKFactory>> m_aFactories;

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

    Awesomium::WebCore *m_pWebCore;
    Awesomium::WebSession *m_pWebSession;
    std::map<Awesomium::WebView *, CComPtr<IUnknown>> m_aWebSurfaces;

    std::map<GEKHASH, CComPtr<IUnknown>> m_aTextures;
    std::map<GEKHASH, CComPtr<IUnknown>> m_aMaterials;
    std::map<GEKHASH, CComPtr<IUnknown>> m_aPrograms;
    std::map<GEKHASH, CComPtr<IGEKModel>> m_aModels;
    std::map<GEKHASH, CComPtr<IGEKRenderFilter>> m_aFilters;
    std::map<GEKHASH, PASS> m_aPasses;

    IGEKEntity *m_pViewer;
    frustum m_kFrustum;
    ENGINEBUFFER m_kEngineBuffer;

    PASS *m_pCurrentPass;
    IGEKRenderFilter *m_pCurrentFilter;
    std::map<PASS *, INT32> m_aCurrentPasses;

    std::map<IGEKModel *, std::vector<IGEKModel::INSTANCE>> m_aVisibleModels;
    std::vector<LIGHT> m_aVisibleLights;

private:
    HRESULT LoadPass(LPCWSTR pName);

public:
    CGEKRenderManager(void);
    virtual ~CGEKRenderManager(void);
    DECLARE_UNKNOWN(CGEKRenderManager);

    // IGEKSceneObserver
    STDMETHOD(OnLoadEnd)                    (THIS_ HRESULT hRetVal);

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

    // IGEKModelManager
    STDMETHOD(LoadCollision)                (THIS_ LPCWSTR pName, LPCWSTR pParams, IGEKCollision **ppCollision);
    STDMETHOD(LoadModel)                    (THIS_ LPCWSTR pName, LPCWSTR pParams, IUnknown **ppModel);

    // IGEKViewManager
    STDMETHOD(SetViewer)                    (THIS_ IGEKEntity *pEntity);
    STDMETHOD_(IGEKEntity *, GetViewer)     (THIS);
    STDMETHOD(EnablePass)                   (THIS_ LPCWSTR pName, INT32 nPriority);

    // IGEKRenderManager
    STDMETHOD_(void, Free)                  (THIS);
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD_(void, SetResource)           (THIS_ IGEKVideoContextSystem *pSystem, UINT32 nStage, IUnknown *pTexture);
    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppBuffer);
    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes);
    STDMETHOD_(void, DrawLights)            (THIS_ std::function<void(void)> OnLightBatch);
    STDMETHOD_(void, DrawOverlay)           (THIS);
    STDMETHOD_(void, Render)                (THIS_ bool bUpdateScreen);

    // Awesomium::DataSource
    void OnRequest(int nRequestID, const Awesomium::ResourceRequest &kRequest, const Awesomium::WebString &kPath);

    // Awesomium::JSMethodHandler
    void OnMethodCall(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs);
    Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs);

    // Awesomium::SurfaceFactory
    Awesomium::Surface *CreateSurface(Awesomium::WebView *pView, int nXSize, int nYSize);
    void DestroySurface(Awesomium::Surface *pSurface);

    // Awesomium::WebViewListener::View
    void  OnChangeTitle(Awesomium::WebView *pCaller, const Awesomium::WebString &kTitle);
    void  OnChangeAddressBar(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL);
    void  OnChangeTooltip(Awesomium::WebView *pCaller, const Awesomium::WebString &kTooltip);
    void  OnChangeTargetURL(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL);
    void  OnChangeCursor(Awesomium::WebView *pCaller, Awesomium::Cursor eCursor);
    void  OnChangeFocus(Awesomium::WebView *pCaller, Awesomium::FocusedElementType eFocusedType);
    void  OnAddConsoleMessage(Awesomium::WebView *pCaller, const Awesomium::WebString &kMessage, int nLineNumber, const Awesomium::WebString &kSource);
    void  OnShowCreatedWebView(Awesomium::WebView *pCaller, Awesomium::WebView *pNewView, const Awesomium::WebURL &kOpenerURL, const Awesomium::WebURL &kTargetURL, const Awesomium::Rect &nInitialPosition, bool bIsPopup);
};