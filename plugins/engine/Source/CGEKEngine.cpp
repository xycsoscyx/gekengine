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
    INTERFACE_LIST_ENTRY_COM(IGEKEngine)
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
            CheckInput(WM_MOUSEWHEEL, ((float(nDelta) / float(WHEEL_DELTA)) * 4));
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
        auto pIterator = m_aInputBindings.find(nKey);
        if (pIterator != m_aInputBindings.end())
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnState, std::placeholders::_1, (*pIterator).second, bState)));
        }
    }
}

void CGEKEngine::CheckInput(UINT32 nKey, float nValue)
{
    if (!m_bConsoleOpen)
    {
        auto pIterator = m_aInputBindings.find(nKey);
        if (pIterator != m_aInputBindings.end())
        {
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnValue, std::placeholders::_1, (*pIterator).second, nValue)));
        }
    }
}

STDMETHODIMP CGEKEngine::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKEngine, GetUnknown());
    if (SUCCEEDED(hRetVal))
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
    }

    WNDCLASSEX kClass = { 0 };
    if (!GetClassInfoEx(GetModuleHandle(nullptr), L"GEKvX_Engine_314159", &kClass))
    {
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
        hRetVal = (RegisterClass(&kClass) ? S_OK : E_FAIL);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = E_FAIL;
        m_hWindow = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, 0, 0, 1, 1, 0, nullptr, GetModuleHandle(nullptr), 0);
        if (m_hWindow != nullptr)
        {
            hRetVal = S_OK;
            SetWindowLong(m_hWindow, GWL_USERDATA, (long)this);
            ShowWindow(m_hWindow, SW_HIDE);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKVideoSystem, IID_PPV_ARGS(&m_spVideoSystem));
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKAudioSystem, IID_PPV_ARGS(&m_spAudioSystem));
        if (SUCCEEDED(hRetVal) && m_spAudioSystem)
        {
            hRetVal = m_spAudioSystem->Initialize(m_hWindow);
        }
    }

    if (SUCCEEDED(hRetVal))
    {
#ifdef _DEBUG
        m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.debug.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#else
        m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#endif
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKEngine::Destroy(void)
{
    xmlCleanupParser();
    m_spAudioSystem.Release();
    m_spVideoSystem.Release();
    GetContext()->RemoveCachedClass(CLSID_GEKEngine);
}

STDMETHODIMP_(void) CGEKEngine::Run(void)
{
    UINT32 nXSize = 800;
    UINT32 nYSize = 600;
    bool bWindowed = false;

    CLibXMLDoc kDocument;
    if (SUCCEEDED(kDocument.Load(L"%root%\\config.xml")))
    {
        CLibXMLNode kRoot = kDocument.GetRoot();
        if (kRoot && kRoot.GetType().CompareNoCase(L"config") == 0 && kRoot.HasChildElement(L"video"))
        {
            CLibXMLNode kVideo = kRoot.FirstChildElement(L"video");
            if (kVideo)
            {
                if (kVideo.HasAttribute(L"xsize"))
                {
                    nXSize = StrToUINT32(kVideo.GetAttribute(L"xsize"));
                }

                if (kVideo.HasAttribute(L"ysize"))
                {
                    nYSize = StrToUINT32(kVideo.GetAttribute(L"ysize"));
                }

                if (kVideo.HasAttribute(L"windowed"))
                {
                    bWindowed = StrToBoolean(kVideo.GetAttribute(L"windowed"));
                }
            }
        }
    }

    m_aInputBindings[VK_UP] = L"forward";
    m_aInputBindings[VK_DOWN] = L"backward";
    m_aInputBindings[VK_LEFT] = L"strafe_left";
    m_aInputBindings[VK_RIGHT] = L"strafe_right";
    m_aInputBindings['W'] = L"forward";
    m_aInputBindings['S'] = L"backward";
    m_aInputBindings['A'] = L"strafe_left";
    m_aInputBindings['D'] = L"strafe_right";
    m_aInputBindings['Q'] = L"rise";
    m_aInputBindings['Z'] = L"fall";
    m_aInputBindings[WM_MOUSEWHEEL] = L"height";
    m_aInputBindings[WM_TURN] = L"turn";
    m_aInputBindings[WM_TILT] = L"tilt";

    m_aInputBindings[VK_ESCAPE] = L"quit";

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
    ShowWindow(m_hWindow, SW_SHOW);
    UpdateWindow(m_hWindow);

    m_bWindowActive = true;
    HRESULT hRetVal = m_spVideoSystem->Initialize(m_hWindow, nXSize, nYSize, bWindowed);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKPopulationSystem, IID_PPV_ARGS(&m_spPopulationManager));
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderSystem, IID_PPV_ARGS(&m_spRenderManager));
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = CGEKObservable::AddObserver(m_spRenderManager, (IGEKRenderObserver *)GetUnknown());
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = m_spPopulationManager->LoadSystems();
    }

    if (SUCCEEDED(hRetVal))
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
                        CheckInput(WM_TURN, float(nCursorMoveX));
                        CheckInput(WM_TILT, float(nCursorMoveY));
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
        m_spPopulationManager->Free();
        CGEKObservable::RemoveObserver(m_spRenderManager, (IGEKRenderObserver *)GetUnknown());
        if (m_spPopulationManager)
        {
            m_spPopulationManager->FreeSystems();
        }
    }

    m_spRenderManager.Release();
    m_spPopulationManager.Release();
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
    CComQIPtr<IGEK2DVideoSystem> spVideoSystem(GetContext()->GetCachedClass<IGEK2DVideoSystem>(CLSID_GEKVideoSystem));
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
