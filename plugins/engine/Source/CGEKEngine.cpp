#include "CGEKEngine.h"
#include "IGEKRenderSystem.h"
#include "IGEKPopulationSystem.h"
#include <libxml/parserInternals.h>
#include <windowsx.h>
#include <algorithm>
#include <atlpath.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

#include "resource.h"

#define WM_TURN             (WM_USER + 0)
#define WM_TILT             (WM_USER + 1)

CGEKConfigGroup::CGEKConfigGroup(void)
{
}

CGEKConfigGroup::~CGEKConfigGroup(void)
{
}

void CGEKConfigGroup::Load(CLibXMLNode &kNode)
{
    m_strText = kNode.GetText();
    kNode.ListAttributes([&](LPCWSTR pName, LPCWSTR pValue) -> void
    {
        m_aValues[pName] = pValue;
    });

    CLibXMLNode &kChild = kNode.FirstChildElement();
    while (kChild)
    {
        CComPtr<CGEKConfigGroup> spChild = new CGEKConfigGroup();
        if (spChild)
        {
            spChild->Load(kChild);
            m_aGroups.insert(std::make_pair(kChild.GetType(), spChild));
        }

        kChild = kChild.NextSiblingElement();
    };
}

void CGEKConfigGroup::Save(CLibXMLNode &kNode)
{
    kNode.SetText(m_strText);
    for (auto &kPair : m_aValues)
    {
        kNode.SetAttribute(kPair.first, kPair.second);
    }

    for (auto &kPair : m_aGroups)
    {
        CLibXMLNode &kChild = kNode.CreateChildElement(kPair.first);
        kPair.second->Save(kChild);
    }
}

STDMETHODIMP_(LPCWSTR) CGEKConfigGroup::GetText(void)
{
    return m_strText;
}

STDMETHODIMP_(bool) CGEKConfigGroup::HasGroup(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, false);

    bool bExists = false;
    auto pIterator = m_aGroups.find(pName);
    if (pIterator != m_aGroups.end())
    {
        bExists = true;
    }

    return bExists;
}

STDMETHODIMP_(IGEKConfigGroup *) CGEKConfigGroup::GetGroup(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, nullptr);

    auto pIterator = m_aGroups.find(pName);
    if (pIterator == m_aGroups.end())
    {
        m_aGroups.insert(std::make_pair(pName, new CGEKConfigGroup()));
        pIterator = m_aGroups.find(pName);
    }

    return (*pIterator).second;
}

STDMETHODIMP_(void) CGEKConfigGroup::ListGroups(std::function<void(LPCWSTR, IGEKConfigGroup*)> OnGroup)
{
    for (auto &kPair : m_aGroups)
    {
        OnGroup(kPair.first, kPair.second);
    }
}

STDMETHODIMP_(bool) CGEKConfigGroup::HasValue(LPCWSTR pName)
{
    REQUIRE_RETURN(pName, false);

    bool bExists = false;
    auto pIterator = m_aValues.find(pName);
    if (pIterator != m_aValues.end())
    {
        bExists = true;
    }

    return bExists;
}

STDMETHODIMP_(LPCWSTR) CGEKConfigGroup::GetValue(LPCWSTR pName, LPCWSTR pDefault)
{
    REQUIRE_RETURN(pName, pDefault);

    auto pIterator = m_aValues.find(pName);
    if (pIterator == m_aValues.end())
    {
        m_aValues[pName] = pDefault;
        pIterator = m_aValues.find(pName);
    }

    return (*pIterator).second.GetString();
}

STDMETHODIMP_(void) CGEKConfigGroup::ListValues(std::function<void(LPCWSTR, LPCWSTR)> OnValue)
{
    for (auto &kPair : m_aValues)
    {
        OnValue(kPair.first, kPair.second);
    }
}

BEGIN_INTERFACE_LIST(CGEKConfigGroup)
    INTERFACE_LIST_ENTRY_COM(IGEKConfigGroup)
END_INTERFACE_LIST_UNKNOWN

HCURSOR LoadAnimatedCursor(HINSTANCE hInstance, UINT nID, LPCTSTR pszResouceType)
{
    HCURSOR hCursor = nullptr;
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(nID), pszResouceType);
    if (hResource != nullptr)
    {
        DWORD dwResourceSize = SizeofResource(hInstance, hResource);
        if (dwResourceSize > 0)
        {
            HGLOBAL hRsrcGlobal = LoadResource(hInstance, hResource);
            if (hRsrcGlobal)
            {
                LPBYTE pResource = (LPBYTE)LockResource(hRsrcGlobal);
                if (pResource)
                {
                    hCursor = (HCURSOR)CreateIconFromResource(pResource, dwResourceSize, FALSE, 0x00030000);
                    UnlockResource(pResource);
                }

                FreeResource(hRsrcGlobal);
            }
        }
    }

    return hCursor;
}

BEGIN_INTERFACE_LIST(CGEKEngine)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKGameApplication)
    INTERFACE_LIST_ENTRY_COM(IGEKEngineCore)
    INTERFACE_LIST_ENTRY_COM(IGEKInputManager)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKEngine)

LRESULT CALLBACK CGEKEngine::WindowProc(HWND hWindow, UINT32 nMessage, WPARAM wParam, LPARAM lParam)
{
    CGEKEngine *pEngine = (CGEKEngine *)GetWindowLong(hWindow, GWL_USERDATA);
    if (pEngine == nullptr)
    {
        return DefWindowProc(hWindow, nMessage, wParam, lParam);
    }

    return pEngine->WindowProc(nMessage, wParam, lParam);
}

LRESULT CGEKEngine::WindowProc(UINT32 nMessage, WPARAM wParam, LPARAM lParam)
{
    switch (nMessage)
    {

    case WM_CLOSE:
        m_bIsClosed = true;
        DestroyWindow(m_hWindow);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SETCURSOR:
        if (m_bConsoleOpen)
        {
            SetCursor(m_hCursorPointer);
        }
        else
        {
            SetCursor(nullptr);
        }

        return 1;

    case WM_ACTIVATE:
        if (HIWORD(wParam))
        {
            m_bWindowActive = false;
            m_kTimer.Pause(!m_bWindowActive);
        }
        else
        {
            switch (LOWORD(wParam))
            {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
                m_bWindowActive = true;
                m_kTimer.Pause(!m_bWindowActive);
                break;

            case WA_INACTIVE:
                m_bWindowActive = false;
                m_kTimer.Pause(!m_bWindowActive);
                break;
            };
        }

        return 1;

    case WM_CHAR:
        if (m_bConsoleOpen)
        {
            switch (wParam)
            {
            case 0x08: // backspace
                if (m_strConsole.GetLength() > 0)
                {
                    m_strConsole = m_strConsole.Mid(0, m_strConsole.GetLength() - 1);
                }

                break;

            case 0x0A: // linefeed
            case 0x0D: // carriage return
                if (true)
                {
                    int nPosition = 0;
                    CStringW strCommand = m_strConsole.Tokenize(L" ", nPosition);

                    std::vector<CStringW> aParams;
                    while (nPosition >= 0 && nPosition < m_strConsole.GetLength())
                    {
                        aParams.push_back(m_strConsole.Tokenize(L" ", nPosition));
                    };

                    RunCommand(strCommand, aParams);
                }

                m_strConsole.Empty();
                break;

            case 0x1B: // escape
                m_strConsole.Empty();
                break;

            case 0x09: // tab
                break;

            default:
                if (wParam != '`')
                {
                    m_strConsole += (WCHAR)wParam;
                }

                break;
            };
        }

        break;

    case WM_KEYDOWN:
        CheckInput(wParam, true);
        return 1;

    case WM_KEYUP:
        CheckInput(wParam, false);
        return 1;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        CheckInput(nMessage, true);
        return 1;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        CheckInput(nMessage, false);
        return 1;

    case WM_MOUSEWHEEL:
        if (true)
        {
            INT32 nDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (nDelta > 0)
            {
                CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"rise", ((float(nDelta) / float(WHEEL_DELTA)) * 4))));
            }
            else if (nDelta < 0)
            {
                CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"fall", -((float(nDelta) / float(WHEEL_DELTA)) * 4))));
            }
        }

        return 1;

    case WM_SYSCOMMAND:
        if (SC_KEYMENU == (wParam & 0xFFF0))
        {
            m_spVideoSystem->Resize(m_spVideoSystem->GetXSize(), m_spVideoSystem->GetYSize(), !m_spVideoSystem->IsWindowed());
            return 1;
        }

        break;
    };

    return DefWindowProc(m_hWindow, nMessage, wParam, lParam);
}

CGEKEngine::CGEKEngine(void)
    : m_hWindow(nullptr)
    , m_bIsClosed(false)
    , m_bIsRunning(false)
    , m_bWindowActive(false)
    , m_bConsoleOpen(false)
    , m_nConsolePosition(0.0f)
    , m_hCursorPointer(nullptr)
{
}

CGEKEngine::~CGEKEngine(void)
{
    if (m_hCursorPointer)
    {
        DestroyIcon(m_hCursorPointer);
    }
}

void CGEKEngine::CheckInput(UINT32 nKey, bool bState)
{
    if (nKey == 0xC0 && !bState)
    {
        m_bConsoleOpen = !m_bConsoleOpen;
    }
    else if (!m_bConsoleOpen)
    {
        switch (nKey)
        {
        case 'W':
        case VK_UP:
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"forward", bState)));
            break;

        case 'S':
        case VK_DOWN:
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"backward", bState)));
            break;

        case 'A':
        case VK_LEFT:
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"strafe_left", bState)));
            break;

        case 'D':
        case VK_RIGHT:
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"strafe_right", bState)));
            break;

        case 'Q':
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"rise", bState)));
            break;

        case 'Z':
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, L"fall", bState)));
            break;
        };
    }
}

STDMETHODIMP_(void) CGEKEngine::Run(void)
{
    LIBXML_TEST_VERSION;
    static xmlExternalEntityLoader kDefaultXMLLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader([](LPCSTR pURL, LPCSTR pID, xmlParserCtxtPtr pContext) -> xmlParserInputPtr
    {
        xmlParserInputPtr pReturn = nullptr;
        if (pID == nullptr)
        {
            pReturn = xmlNewInputFromFile(pContext, pURL);
        }
        else
        {
            CStringW strID(CA2W(pID, CP_UTF8));
            CStringA strFileName = CW2A(GEKParseFileName(strID), CP_UTF8);
            pReturn = xmlNewInputFromFile(pContext, strFileName);
        }

        if (kDefaultXMLLoader != nullptr && pReturn == nullptr)
        {
            pReturn = kDefaultXMLLoader(pURL, pID, pContext);
        }

        return pReturn;
    });

    m_spConfig = new CGEKConfigGroup();
    if (m_spConfig)
    {
        CLibXMLDoc kLoadConfig;
        if (SUCCEEDED(kLoadConfig.Load(L"%root%\\config.xml")))
        {
            CLibXMLNode kRoot = kLoadConfig.GetRoot();
            if (kRoot && kRoot.GetType().CompareNoCase(L"config") == 0)
            {
                m_spConfig->Load(kRoot);
            }
        }

#ifdef _DEBUG
        m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.debug.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#else
        m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#endif

        WNDCLASS kClass;
        kClass.style = 0;
        kClass.lpfnWndProc = WindowProc;
        kClass.cbClsExtra = 0;
        kClass.cbWndExtra = 0;
        kClass.hInstance = GetModuleHandle(nullptr);
        kClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
        kClass.hCursor = nullptr;
        kClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        kClass.lpszMenuName = nullptr;
        kClass.lpszClassName = L"GEKvX_Engine_314159";
        if (RegisterClass(&kClass))
        {
            UINT32 nXSize = StrToUINT32(GetConfig()->GetGroup(L"display")->GetValue(L"xsize", L"800"));
            UINT32 nYSize = StrToUINT32(GetConfig()->GetGroup(L"display")->GetValue(L"ysize", L"600"));
            bool bWindowed = StrToBoolean(GetConfig()->GetGroup(L"display")->GetValue(L"windowed", L"true"));

            RECT kRect;
            kRect.left = 0;
            kRect.top = 0;
            kRect.right = nXSize;
            kRect.bottom = nYSize;
            AdjustWindowRect(&kRect, WS_OVERLAPPEDWINDOW, false);
            int nWindowXSize = (kRect.right - kRect.left);
            int nWindowYSize = (kRect.bottom - kRect.top);
            int nCenterX = (bWindowed ? (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((kRect.right - kRect.left) / 2) : 0);
            int nCenterY = (bWindowed ? (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((kRect.bottom - kRect.top) / 2) : 0);
            m_hWindow = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, nCenterX, nCenterY, nWindowXSize, nWindowYSize, 0, nullptr, GetModuleHandle(nullptr), 0);
            if (m_hWindow != nullptr)
            {
                SetWindowLong(m_hWindow, GWL_USERDATA, (long)this);
                if (SUCCEEDED(GetContext()->CreateInstance(CLSID_GEKVideoSystem, IID_PPV_ARGS(&m_spVideoSystem))) && m_spVideoSystem &&
                    SUCCEEDED(GetContext()->CreateInstance(CLSID_GEKAudioSystem, IID_PPV_ARGS(&m_spAudioSystem))) && m_spAudioSystem &&
                    SUCCEEDED(m_spAudioSystem->Initialize(m_hWindow)))
                {
                    ShowWindow(m_hWindow, SW_SHOW);
                    UpdateWindow(m_hWindow);

                    m_bWindowActive = true;
                    if (SUCCEEDED(m_spVideoSystem->Initialize(m_hWindow, nXSize, nYSize, bWindowed)))
                    {
                        if (SUCCEEDED(GetContext()->CreateInstance(CLSID_GEKPopulationSystem, IID_PPV_ARGS(&m_spPopulationManager))) && m_spPopulationManager &&
                            SUCCEEDED(GetContext()->CreateInstance(CLSID_GEKRenderSystem, IID_PPV_ARGS(&m_spRenderManager))) && m_spRenderManager &&
                            SUCCEEDED(CGEKObservable::AddObserver(m_spRenderManager, (IGEKRenderObserver *)GetUnknown())) &&
                            SUCCEEDED(m_spPopulationManager->Initialize(this)) &&
                            SUCCEEDED(m_spRenderManager->Initialize(this)))
                        {
                            RunCommand(L"load", { L"demo" });

                            m_bIsRunning = true;
                            MSG kMessage = { 0 };
                            while (m_bIsRunning && !m_bIsClosed)
                            {
                                while (PeekMessage(&kMessage, nullptr, 0U, 0U, PM_REMOVE))
                                {
                                    TranslateMessage(&kMessage);
                                    DispatchMessage(&kMessage);
                                };

                                if (m_bWindowActive)
                                {
                                    m_kTimer.Update();
                                    float nFrameTime = float(m_kTimer.GetUpdateTime());
                                    if (m_bConsoleOpen)
                                    {
                                        m_nConsolePosition = min(1.0f, (m_nConsolePosition + (nFrameTime * 4.0f)));
                                    }
                                    else
                                    {
                                        m_nConsolePosition = max(0.0f, (m_nConsolePosition - (nFrameTime * 4.0f)));

                                        POINT kCursor;
                                        GetCursorPos(&kCursor);

                                        RECT kWindow;
                                        GetWindowRect(m_hWindow, &kWindow);
                                        INT32 nCenterX = (kWindow.left + ((kWindow.right - kWindow.left) / 2));
                                        INT32 nCenterY = (kWindow.top + ((kWindow.bottom - kWindow.top) / 2));
                                        SetCursorPos(nCenterX, nCenterY);

                                        INT32 nCursorMoveX = ((kCursor.x - nCenterX) / 2);
                                        INT32 nCursorMoveY = ((kCursor.y - nCenterY) / 2);
                                        if (nCursorMoveX != 0 || nCursorMoveY != 0)
                                        {
                                            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"turn", float(nCursorMoveX))));
                                            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, L"tilt", float(nCursorMoveY))));
                                        }

                                        UINT32 nFrame = 3;
                                        m_nTimeAccumulator += nFrameTime;
                                        while (m_nTimeAccumulator > (1.0 / 30.0))
                                        {
                                            m_nTotalTime += (1.0f / 30.0f);
                                            m_spPopulationManager->Update(float(m_nTotalTime), (1.0f / 30.0f));
                                            if (--nFrame == 0)
                                            {
                                                m_nTimeAccumulator = 0.0f;
                                            }
                                            else
                                            {
                                                m_nTimeAccumulator -= (1.0 / 30.0);
                                            }
                                        };
                                    }

                                    m_spRenderManager->Render();
                                }
                            };
                            
                            m_bWindowActive = false;
                            CGEKObservable::RemoveObserver(m_spRenderManager, (IGEKRenderObserver *)GetUnknown());
                        }

                        m_spRenderManager.Release();
                        m_spPopulationManager.Release();
                    }
                }

                CLibXMLDoc kSaveConfig;
                if (SUCCEEDED(kSaveConfig.Create(L"config")))
                {
                    m_spConfig->Save(kSaveConfig.GetRoot());
                    kSaveConfig.Save(L"%root%\\config.xml");
                }

                m_spAudioSystem.Release();
                m_spVideoSystem.Release();
                DestroyWindow(m_hWindow);
            }
        }
    }

    xmlCleanupParser();
}

STDMETHODIMP_(IGEKConfigGroup *) CGEKEngine::GetConfig(void)
{
    REQUIRE_RETURN(m_spConfig, nullptr);
    return m_spConfig;
}

STDMETHODIMP_(IGEK3DVideoSystem *) CGEKEngine::GetVideoSystem(void)
{
    REQUIRE_RETURN(m_spVideoSystem, nullptr);
    return m_spVideoSystem;
}

STDMETHODIMP_(IGEKSceneManager *) CGEKEngine::GetSceneManager(void)
{
    REQUIRE_RETURN(m_spPopulationManager, nullptr);
    return dynamic_cast<IGEKSceneManager *>((IGEKPopulationSystem *)m_spPopulationManager);
}

STDMETHODIMP_(IGEKRenderManager *) CGEKEngine::GetRenderManager(void)
{
    REQUIRE_RETURN(m_spRenderManager, nullptr);
    return dynamic_cast<IGEKRenderManager *>((IGEKRenderSystem *)m_spRenderManager);
}

STDMETHODIMP_(IGEKProgramManager *) CGEKEngine::GetProgramManager(void)
{
    REQUIRE_RETURN(m_spConfig, nullptr);
    return dynamic_cast<IGEKProgramManager *>((IGEKRenderSystem *)m_spRenderManager);
}

STDMETHODIMP_(IGEKMaterialManager *) CGEKEngine::GetMaterialManager(void)
{
    REQUIRE_RETURN(m_spConfig, nullptr);
    return dynamic_cast<IGEKMaterialManager *>((IGEKRenderSystem *)m_spRenderManager);
}

STDMETHODIMP_(void) CGEKEngine::ShowMessage(GEKMESSAGETYPE eType, LPCWSTR pSystem, LPCWSTR pMessage, ...)
{
    va_list pArgs;
    CStringW strMessage;
    va_start(pArgs, pMessage);
    strMessage.FormatV(pMessage, pArgs);
    va_end(pArgs);

    if (pSystem)
    {
        OutputDebugString(FormatString(L"[%s] %s\r\n", pSystem, strMessage.GetString()));
    }
    else
    {
        OutputDebugString(FormatString(L"%s\r\n", strMessage.GetString()));
    }

    m_aConsoleLog.push_front(std::make_pair(eType, strMessage));
    if (m_aConsoleLog.size() > 100)
    {
        m_aConsoleLog.pop_back();
    }

    if (pSystem)
    {
        std::list<GEKMESSAGE> &aSystemLog = m_aSystemLogs[pSystem];
        aSystemLog.push_front(std::make_pair(eType, strMessage));
        if (aSystemLog.size() > 100)
        {
            aSystemLog.pop_back();
        }
    }
}

STDMETHODIMP_(void) CGEKEngine::RunCommand(LPCWSTR pCommand, const std::vector<CStringW> &aParams)
{
    if (_wcsicmp(pCommand, L"quit") == 0)
    {
        ShowMessage(GEKMESSAGE_NORMAL, L"command", L"Quitting...");
        m_bIsRunning = false;
    }
    else if (_wcsicmp(pCommand, L"load") == 0 && aParams.size() == 1)
    {
        ShowMessage(GEKMESSAGE_NORMAL, L"command", L"Loading Level (%s)...", aParams[0].GetString());
        if (FAILED(m_spPopulationManager->Load(aParams[0])))
        {
            m_spPopulationManager->Free();
        }
        else
        {
            m_bConsoleOpen = false;
            m_nTotalTime = 0.0;
            m_nTimeAccumulator = 0.0f;
            m_kTimer.Reset();
        }
    }
    else if (_wcsicmp(pCommand, L"setresolution") == 0 && aParams.size() == 3)
    {
        ShowMessage(GEKMESSAGE_NORMAL, L"command", L"Setting Resolution (%sx%s %s)...", aParams[0].GetString(), aParams[1].GetString(), (StrToBoolean(aParams[2]) ? L"Windowed" : L"Fullscreen"));
        UINT32 nXSize = StrToUINT32(aParams[0]);
        UINT32 nYSize = StrToUINT32(aParams[1]);
        bool bWindowed = StrToBoolean(aParams[2]);

        RECT kRect;
        kRect.left = 0;
        kRect.top = 0;
        kRect.right = nXSize;
        kRect.bottom = nYSize;
        AdjustWindowRect(&kRect, WS_OVERLAPPEDWINDOW, false);
        int nWindowXSize = (kRect.right - kRect.left);
        int nWindowYSize = (kRect.bottom - kRect.top);
        int nCenterX = (bWindowed ? (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((kRect.right - kRect.left) / 2) : 0);
        int nCenterY = (bWindowed ? (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((kRect.bottom - kRect.top) / 2) : 0);
        SetWindowPos(m_hWindow, nullptr, nCenterX, nCenterY, nWindowXSize, nWindowYSize, 0);
        if (FAILED(m_spVideoSystem->Resize(nXSize, nYSize, bWindowed)))
        {
            m_bIsRunning = false;
        }
    }
    else if (_wcsicmp(pCommand, L"showlog") == 0 && aParams.size() == 1)
    {
        ShowMessage(GEKMESSAGE_NORMAL, nullptr, L"Showing Log (%s)...", aParams[0].GetString());

        std::list<GEKMESSAGE> &aSystem = m_aSystemLogs[aParams[0]];
        for (auto &kMessage : aSystem)
        {
            ShowMessage(kMessage.first, nullptr, kMessage.second);
        }
    }
    else
    {
        ShowMessage(GEKMESSAGE_NORMAL, L"command", L"Unknown Command: %s", pCommand);
        return;
    }

    m_aCommandLog.push_back(std::make_pair(pCommand, aParams));
}

STDMETHODIMP_(void) CGEKEngine::OnRenderOverlay(void)
{
    CComQIPtr<IGEK2DVideoSystem> spVideoSystem(m_spVideoSystem);
    if (spVideoSystem)
    {
        spVideoSystem->Begin();

        float nXSize = float(m_spVideoSystem->GetXSize());
        float nYSize = float(m_spVideoSystem->GetYSize());
        float nHalfHeight = (nYSize* 0.5f);

        CComPtr<IUnknown> spBackground;
        spVideoSystem->CreateBrush({ { 0.0f, float4(0.5f, 0.0f, 0.0f, 1.0f) }, { 1.0f, float4(0.25f, 0.0f, 0.0f, 1.0f) } }, { 0.0f, 0.0f, 0.0f, nHalfHeight }, &spBackground);

        CComPtr<IUnknown> spForeground;
        spVideoSystem->CreateBrush({ { 0.0f, float4(0.0f, 0.0f, 0.0f, 1.0f) }, { 1.0f, float4(0.25f, 0.25f, 0.25f, 1.0f) } }, { 0.0f, 0.0f, 0.0f, nHalfHeight }, &spForeground);

        CComPtr<IUnknown> spText;
        spVideoSystem->CreateBrush(float4(1.0f, 1.0f, 1.0f, 1.0f), &spText);

        CComPtr<IUnknown> spFont;
        spVideoSystem->CreateFont(L"Tahoma", 400, GEK2DVIDEO::FONT::NORMAL, 15.0f, &spFont);

        static DWORD nLastTime = 0;
        static std::list<UINT32> aFPS;
        static UINT32 nNumFrames = 0;
        static UINT32 nAverageFPS = 0;

        nNumFrames++;
        DWORD nCurrentTime = GetTickCount();
        if (nCurrentTime - nLastTime > 1000)
        {
            nLastTime = nCurrentTime;
            aFPS.push_back(nNumFrames);
            nNumFrames = 0;

            if (aFPS.size() > 10)
            {
                aFPS.pop_front();
            }

            nAverageFPS = 0;
            for (auto nFPS : aFPS)
            {
                nAverageFPS += nFPS;
            }

            nAverageFPS /= aFPS.size();
        }

        spVideoSystem->SetTransform(float3x2());
        spVideoSystem->DrawText({ 0.0f, nYSize - 15.0f, nXSize, nYSize }, spFont, spText, L"FPS: %d", nAverageFPS);
        if (m_nConsolePosition > 0.0f)
        {
            float nTop = -((1.0f - m_nConsolePosition) * nHalfHeight);

            float3x2 nTransform;
            nTransform.SetTranslation(float2(0.0f, nTop));
            spVideoSystem->SetTransform(nTransform);

            spVideoSystem->DrawRectangle({ 0.0f, 0.0f, nXSize, nHalfHeight }, spBackground, true);
            spVideoSystem->DrawRectangle({ 10.0f, 10.0f, (nXSize - 10.0f), (nHalfHeight - 40.0f) }, spForeground, true);
            spVideoSystem->DrawRectangle({ 10.0f, (nHalfHeight - 30.0f), (nXSize - 10.0f), (nHalfHeight - 10.0f) }, spForeground, true);
            spVideoSystem->DrawText({ 15.0f, (nHalfHeight - 30.0f), (nXSize - 15.0f), (nHalfHeight - 10.0f) }, spFont, spText, m_strConsole + ((GetTickCount() / 500 % 2) ? L"_" : L""));

            CComPtr<IUnknown> spLogTypes[4];
            spVideoSystem->CreateBrush(float4(1.0f, 1.0f, 1.0f, 1.0f), &spLogTypes[0]);
            spVideoSystem->CreateBrush(float4(1.0f, 1.0f, 0.0f, 1.0f), &spLogTypes[1]);
            spVideoSystem->CreateBrush(float4(1.0f, 0.0f, 0.0f, 1.0f), &spLogTypes[2]);
            spVideoSystem->CreateBrush(float4(1.0f, 0.0f, 0.0f, 1.0f), &spLogTypes[3]);

            float nPosition = (nHalfHeight - 40.0f);
            for (auto &kMessage : m_aConsoleLog)
            {
                spVideoSystem->DrawText({ 15.0f, (nPosition - 20.0f), (nXSize - 15.0f), nPosition }, spFont, spLogTypes[kMessage.first], kMessage.second);
                nPosition -= 20.0f;
            }
        }

        spVideoSystem->End();
    }
}
