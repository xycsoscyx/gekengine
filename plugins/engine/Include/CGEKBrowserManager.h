#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include <Awesomium/WebCore.h>
#include <Awesomium/STLHelpers.h>

class CGEKBrowserManager : public CGEKUnknown
                         , public Awesomium::DataSource
                         , public Awesomium::JSMethodHandler
                         , public Awesomium::SurfaceFactory
                         , public Awesomium::WebViewListener::View
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    Awesomium::WebCore *m_pWebCore;
    Awesomium::WebSession *m_pWebSession;
    std::map<Awesomium::WebView *, CComPtr<IUnknown>> m_aWebSurfaces;

public:
    CGEKBrowserManager(void);
    virtual ~CGEKBrowserManager(void);
    DECLARE_UNKNOWN(CGEKBrowserManager);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

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