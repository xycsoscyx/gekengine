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
    INTERFACE_LIST_ENTRY_COM(IGEKSystemObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKGameApplication)
    INTERFACE_LIST_ENTRY_COM(IGEKEngine)
    INTERFACE_LIST_ENTRY_COM(IGEKInputManager)
    INTERFACE_LIST_ENTRY_COM(IGEKRenderObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKEngine)

CGEKEngine::CGEKEngine(void)
    : m_bWindowActive(false)
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

HRESULT CGEKEngine::Load(LPCWSTR pName)
{
    HRESULT hRetVal = m_spPopulationManager->Load(pName);
    if (FAILED(hRetVal))
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

    return hRetVal;
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

        m_bWindowActive = true;
        hRetVal = GetContext()->CreateInstance(CLSID_GEKSystem, IID_PPV_ARGS(&m_spSystem));
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = CGEKObservable::AddObserver(m_spSystem, (IGEKSystemObserver *)GetUnknown());
            hRetVal = m_spSystem->SetSize(640, 480, true);
        }

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
#ifdef _DEBUG
            m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.debug.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#else
            m_hCursorPointer = LoadAnimatedCursor(GetModuleHandle(L"engine.dll"), IDR_CURSOR_POINTER, L"ANIMATEDCURSOR");
#endif
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = Load(L"demo");
        }
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKEngine::Destroy(void)
{
    CGEKObservable::RemoveObserver(m_spRenderManager, (IGEKRenderObserver *)GetUnknown());
    if (m_spPopulationManager)
    {
        m_spPopulationManager->FreeSystems();
    }

    m_spRenderManager.Release();
    m_spPopulationManager.Release();
    CGEKObservable::RemoveObserver(m_spSystem, (IGEKSystemObserver *)this);
    m_spSystem.Release();

    GetContext()->RemoveCachedClass(CLSID_GEKEngine);

    xmlCleanupParser();
}

STDMETHODIMP_(void) CGEKEngine::OnEvent(UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
    switch (nMessage)
    {
    case WM_SETCURSOR:
        if (m_bConsoleOpen)
        {
            SetCursor(m_hCursorPointer);
        }
        else
        {
            SetCursor(nullptr);
        }

        nResult = 1;
        break;

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

        nResult = 1;
        break;

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
                    std::vector<CStringW> aParams;
                    while (nPosition >= 0 && nPosition < m_strConsole.GetLength())
                    {
                        aParams.push_back(m_strConsole.Tokenize(L" ", nPosition));
                    };

                    OnCommand(aParams);
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
        nResult = 1;
        break;

    case WM_KEYUP:
        CheckInput(wParam, false);
        nResult = 1;
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
        CheckInput(nMessage, true);
        nResult = 1;
        break;

    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        CheckInput(nMessage, false);
        nResult = 1;
        break;

    case WM_MOUSEWHEEL:
        if (true)
        {
            INT32 nDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            CheckInput(WM_MOUSEWHEEL, ((float(nDelta) / float(WHEEL_DELTA)) * 4));
        }

        nResult = 1;
        break;
    };
}

STDMETHODIMP_(void) CGEKEngine::OnRun(void)
{
}

STDMETHODIMP_(void) CGEKEngine::OnStop(void)
{
    m_bWindowActive = false;
    m_spPopulationManager->Free();
}

STDMETHODIMP_(void) CGEKEngine::OnStep(void)
{
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
            GetWindowRect(m_spSystem->GetWindow(), &kWindow);
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
}

STDMETHODIMP_(void) CGEKEngine::Run(void)
{
    REQUIRE_VOID_RETURN(m_spSystem);
    m_spSystem->Run();
}

STDMETHODIMP_(void) CGEKEngine::OnMessage(LPCWSTR pMessage, ...)
{
    va_list pArgs;
    CStringW strMessage;
    va_start(pArgs, pMessage);
    strMessage.FormatV(pMessage, pArgs);
    va_end(pArgs);

    OutputDebugString(strMessage + L"\r\n");
    m_aConsoleLog.push_front(strMessage);
    if (m_aConsoleLog.size() > 100)
    {
        m_aConsoleLog.pop_back();
    }
}

STDMETHODIMP_(void) CGEKEngine::OnCommand(const std::vector<CStringW> &aParams)
{
    if (aParams.size() == 1 && aParams[0].CompareNoCase(L"quit") == 0)
    {
        OnMessage(L"Quitting...");
        m_spSystem->Stop();
    }
    else if (aParams.size() == 2 && aParams[0].CompareNoCase(L"load") == 0)
    {
        OnMessage(L"Loading Level...");
        Load(aParams[1]);
    }
    else if (aParams.size() == 4 && aParams[0].CompareNoCase(L"setresolution") == 0)
    {
        OnMessage(L"Setting Resolution (%sx%s %s)...", aParams[1].GetString(), aParams[2].GetString(), (StrToBoolean(aParams[3]) ? L"Windowed" : L"Fullscreen"));
        UINT32 nXSize = StrToUINT32(aParams[1]);
        UINT32 nYSize = StrToUINT32(aParams[2]);
        bool bWindowed = StrToBoolean(aParams[3]);
        if (FAILED(m_spSystem->SetSize(nXSize, nYSize, bWindowed)))
        {
            m_spSystem->Stop();
        }
    }
    else if (aParams.size() > 0)
    {
        OnMessage(L"Unknown Command: %s", aParams[0].GetString());
    }
}

STDMETHODIMP_(void) CGEKEngine::OnRenderOverlay(void)
{
    CComQIPtr<IGEK2DVideoSystem> spVideoSystem(GetContext()->GetCachedClass<IGEK2DVideoSystem>(CLSID_GEKVideoSystem));
    if (spVideoSystem)
    {
        spVideoSystem->Begin();

        float nXSize = float(m_spSystem->GetXSize());
        float nYSize = float(m_spSystem->GetYSize());
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

            float nPosition = (nHalfHeight - 40.0f);
            for (auto &strMessage : m_aConsoleLog)
            {
                spVideoSystem->DrawText({ 15.0f, (nPosition - 20.0f), (nXSize - 15.0f), nPosition }, spFont, spText, strMessage);
                nPosition -= 20.0f;
            }
        }

        spVideoSystem->End();
    }
}
