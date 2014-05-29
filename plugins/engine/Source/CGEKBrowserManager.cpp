#include "CGEKBrowserManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKBrowserManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKBrowserManager)

CGEKBrowserManager::CGEKBrowserManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
    , m_pWebCore(nullptr)
    , m_pWebSession(nullptr)
{
}

CGEKBrowserManager::~CGEKBrowserManager(void)
{
}

STDMETHODIMP CGEKBrowserManager::Initialize(void)
{
    GEKFUNCTION(nullptr);

    HRESULT hRetVal = E_FAIL;
    m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
    {
        m_pWebCore = Awesomium::WebCore::Initialize(Awesomium::WebConfig());
        GEKRESULT(m_pWebCore != nullptr, L"Unable to create Awesomium WebCore instance");
        if (m_pWebCore != nullptr)
        {
            m_pWebSession = m_pWebCore->CreateWebSession(Awesomium::WSLit(""), Awesomium::WebPreferences());
            GEKRESULT(m_pWebSession != nullptr, L"Unable to create Awesomium WebSession instance");
            if (m_pWebSession != nullptr)
            {
                m_pWebSession->AddDataSource(Awesomium::WSLit("Engine"), this);
                m_pWebCore->set_surface_factory(this);
                hRetVal = S_OK;
            }
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKBrowserManager::Destroy(void)
{
    m_aWebSurfaces.clear();
    if (m_pWebSession != nullptr)
    {
        m_pWebSession->Release();
        m_pWebSession = nullptr;
    }

    Awesomium::WebCore::Shutdown();
    m_pWebCore = nullptr;
}

void CGEKBrowserManager::OnRequest(int nRequestID, const Awesomium::ResourceRequest &kRequest, const Awesomium::WebString &kPath)
{
    CStringW strPath((LPCWSTR)kPath.data());

    std::vector<UINT8> aBuffer;
    if (SUCCEEDED(GEKLoadFromFile(L"%root%\\data\\" + strPath, aBuffer)))
    {
        static const std::map<GEKHASH, CStringA> aMimeTypes =
        {
            { L".htm", "text/html", },
            { L".html", "text/html", },
            { L".xml", "text/xml", },
            { L".js", "text/javascript", },
            { L".css", "text/css", },
            { L".png", "image/png", },
            { L".gif", "image/gif", },
            { L".bmp", "image/bmp", },
            { L".jpg", "image/jpg", },
        };

        CStringW strExtension = CPathW(strPath).GetExtension();
        auto pIterator = aMimeTypes.find(strExtension);
        if (pIterator != aMimeTypes.end())
        {
            SendResponse(nRequestID, aBuffer.size(), &aBuffer[0], Awesomium::WSLit((*pIterator).second));
        }
        else
        {
            SendResponse(nRequestID, 0, nullptr, Awesomium::WSLit(""));
        }
    }
    else
    {
        SendResponse(nRequestID, 0, nullptr, Awesomium::WSLit(""));
    }
}

void CGEKBrowserManager::OnMethodCall(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs)
{
    std::vector<CStringW> aParams;
    for (UINT32 nIndex = 0; nIndex < aArgs.size(); nIndex++)
    {
        const Awesomium::JSValue &kValue = aArgs.At(nIndex);
        aParams.push_back((LPCWSTR)kValue.ToString().data());
    }

    if (aParams.size() > 0)
    {
        std::vector<LPCWSTR> aParamPointers(aParams.begin(), aParams.end());
        //m_pEngine->OnCommand((LPCWSTR)kMethodName.data(), &aParamPointers[0], aParamPointers.size());
    }
    else
    {
        //m_pEngine->OnCommand((LPCWSTR)kMethodName.data(), nullptr, 0);
    }
}

Awesomium::JSValue CGEKBrowserManager::OnMethodCallWithReturnValue(Awesomium::WebView *pCaller, unsigned int nRemoteObjectID, const Awesomium::WebString &kMethodName, const Awesomium::JSArray &aArgs)
{
    if (_wcsicmp((LPCWSTR)kMethodName.data(), L"GetResolutions") == 0)
    {
        Awesomium::JSArray aArray;
        std::vector<GEKMODE> akModes = GEKGetDisplayModes()[32];
        for (UINT32 nMode = 0; nMode < akModes.size(); nMode++)
        {
            GEKMODE &kMode = akModes[nMode];

            CStringA strAspect("");
            switch (kMode.GetAspect())
            {
            case _ASPECT_4x3:
                strAspect = ", (4x3)";
                break;

            case _ASPECT_16x9:
                strAspect = ", (16x9)";
                break;

            case _ASPECT_16x10:
                strAspect = ", (16x10)";
                break;
            };

            Awesomium::JSObject kObject;
            kObject.SetProperty(Awesomium::WSLit("Label"), Awesomium::JSValue(Awesomium::WSLit(FormatString("%dx%d%s", kMode.xsize, kMode.ysize, strAspect.GetString()))));
            kObject.SetProperty(Awesomium::WSLit("XSize"), Awesomium::JSValue(int(kMode.xsize)));
            kObject.SetProperty(Awesomium::WSLit("YSize"), Awesomium::JSValue(int(kMode.ysize)));
            aArray.Push(kObject);
        }

        return aArray;
    }
    else if (_wcsicmp((LPCWSTR)kMethodName.data(), L"GetValue") == 0)
    {
        if (aArgs.size() == 2)
        {
            CStringW strGroup = (LPCWSTR)aArgs.At(0).ToString().data();
            CStringW strName = (LPCWSTR)aArgs.At(1).ToString().data();
            CStringW strValue = m_pSystem->GetConfig().GetValue(strGroup, strName, L"");
            return Awesomium::JSValue(Awesomium::WSLit(CW2A(strValue, CP_UTF8)));
        }
    }

    return Awesomium::JSValue();
}

Awesomium::Surface *CGEKBrowserManager::CreateSurface(Awesomium::WebView *pView, int nXSize, int nYSize)
{
    CComPtr<CGEKWebSurface> spWebSurface(new CGEKWebSurface(m_pVideoSystem, nXSize, nYSize));
    GEKRESULT(spWebSurface, L"Unable to allocate new web surface instance");
    if (spWebSurface)
    {
        spWebSurface->QueryInterface(IID_PPV_ARGS(&m_aWebSurfaces[pView]));
        return spWebSurface.Detach();
    }

    return nullptr;
}

void CGEKBrowserManager::DestroySurface(Awesomium::Surface *pSurface)
{
    CComPtr<CGEKWebSurface> spWebSurface;
    spWebSurface.Attach(dynamic_cast<CGEKWebSurface *>(pSurface));
    auto pIterator = std::find_if(m_aWebSurfaces.begin(), m_aWebSurfaces.end(), [&](std::map<Awesomium::WebView *, CComPtr<IUnknown>>::value_type &kPair) -> bool
    {
        return (spWebSurface.IsEqualObject(kPair.second));
    });

    if (pIterator != m_aWebSurfaces.end())
    {
        m_aWebSurfaces.erase(pIterator);
    }
}

void CGEKBrowserManager::OnChangeTitle(Awesomium::WebView *pCaller, const Awesomium::WebString &kTitle)
{
}

void CGEKBrowserManager::OnChangeAddressBar(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL)
{
}

void CGEKBrowserManager::OnChangeTooltip(Awesomium::WebView *pCaller, const Awesomium::WebString &kTooltip)
{
}

void CGEKBrowserManager::OnChangeTargetURL(Awesomium::WebView *pCaller, const Awesomium::WebURL &kURL)
{
}

void CGEKBrowserManager::OnChangeCursor(Awesomium::WebView *pCaller, Awesomium::Cursor eCursor)
{
}

void CGEKBrowserManager::OnChangeFocus(Awesomium::WebView *pCaller, Awesomium::FocusedElementType eFocusedType)
{
}

void CGEKBrowserManager::OnAddConsoleMessage(Awesomium::WebView *pCaller, const Awesomium::WebString &kMessage, int nLineNumber, const Awesomium::WebString &kSource)
{
    CStringW strMessage((LPCWSTR)kMessage.data());
    CStringW strSource((LPCWSTR)kSource.data());
    GEKLOG(L"Browser Console (%s: %d): %s", strSource.GetString(), nLineNumber, strMessage.GetString());
}

void CGEKBrowserManager::OnShowCreatedWebView(Awesomium::WebView *pCaller, Awesomium::WebView *pNewView, const Awesomium::WebURL &kOpenerURL, const Awesomium::WebURL &kTargetURL, const Awesomium::Rect &nInitialPosition, bool bIsPopup)
{
}
