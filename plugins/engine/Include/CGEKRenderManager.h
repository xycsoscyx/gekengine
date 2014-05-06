#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKRenderManager.h"
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
                        , public CGEKContextUser
                        , public CGEKSystemUser
                        , public CGEKVideoSystemUser
                        , public CGEKEngineUser
                        , public IGEKContextObserver
                        , public IGEKVideoObserver
                        , public IGEKSystemObserver
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
        INT32 m_nPriority;
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
        float4x4 m_nViewMatrix;
        float4x4 m_nProjectionMatrix;
        float4x4 m_nTransformMatrix;
        float3 m_nCameraPosition;
        float m_nCameraViewDistance;
        float2 m_nCameraView;
        float2 m_nCameraSize;
    };

private:
    std::list<CComPtr<IGEKFactory>> m_aFactories;

    CComPtr<IUnknown> m_spFrameEvent;

    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IGEKVideoBuffer> m_spVertexBuffer;
    CComPtr<IGEKVideoBuffer> m_spIndexBuffer;

    CComPtr<IUnknown> m_spPointSampler;
    CComPtr<IUnknown> m_spLinearSampler;

    CComPtr<IGEKVideoBuffer> m_spOrthoBuffer;
    CComPtr<IGEKVideoBuffer> m_spEngineBuffer;
    CComPtr<IGEKVideoBuffer> m_spLightBuffer;
    UINT32 m_nNumLightInstances;

    Awesomium::WebCore *m_pWebCore;
    Awesomium::WebSession *m_pWebSession;

    std::map<GEKHASH, Awesomium::WebView *> m_aWebViews;
    std::map<Awesomium::WebView *, CComPtr<IUnknown>> m_aWebSurfaces;
    std::list<Awesomium::WebView *> m_aGUIViews;

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
    std::map<PASS *, UINT32> m_aCurrentPasses;

    concurrency::concurrent_unordered_multimap<IGEKModel *, IGEKModel::INSTANCE> m_aCurrentModels;
    concurrency::concurrent_vector<LIGHT> m_aCurrentLights;

    std::map<IGEKModel *, std::vector<IGEKModel::INSTANCE>> m_aCulledModels;
    std::vector<LIGHT> m_aCulledLights;

private:
    HRESULT LoadPass(LPCWSTR pName);

public:
    CGEKRenderManager(void);
    virtual ~CGEKRenderManager(void);
    DECLARE_UNKNOWN(CGEKRenderManager);

    // IGEKContextObserver
    STDMETHOD(OnRegistration)               (THIS_ IUnknown *pObject);

    // IGEKSystemObserver
    STDMETHOD_(void, OnEvent)               (THIS_ UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult);

    // IGEKVideoObserver
    STDMETHOD_(void, OnPreReset)            (THIS);
    STDMETHOD(OnPostReset)                  (THIS);

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
    STDMETHOD_(void, DrawLight)             (THIS_ IGEKEntity *pEntity, const GEKLIGHT &kLight);
    STDMETHOD_(void, DrawModel)             (THIS_ IGEKEntity *pEntity, IUnknown *pModel);
    STDMETHOD(EnablePass)                   (THIS_ LPCWSTR pName);
    STDMETHOD_(void, CaptureMouse)          (THIS_ bool bCapture);

    // IGEKRenderManager
    STDMETHOD_(void, BeginLoad)             (THIS);
    STDMETHOD_(void, EndLoad)               (THIS_ HRESULT hRetVal);
    STDMETHOD_(void, Free)                  (THIS);
    STDMETHOD(LoadResource)                 (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD_(void, SetResource)           (THIS_ UINT32 nStage, IUnknown *pTexture);
    STDMETHOD(GetBuffer)                    (THIS_ LPCWSTR pName, IUnknown **ppTexture);
    STDMETHOD(GetDepthBuffer)               (THIS_ LPCWSTR pSource, IUnknown **ppBuffer);
    STDMETHOD_(void, DrawScene)             (THIS_ UINT32 nAttributes);
    STDMETHOD_(void, DrawOverlay)           (THIS_ bool bPerLight);
    STDMETHOD(BeginFrame)                   (THIS);
    STDMETHOD_(const frustum &, GetFrustum) (THIS);
    STDMETHOD_(void, EndFrame)              (THIS);

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