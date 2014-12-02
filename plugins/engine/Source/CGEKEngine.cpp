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
    HCURSOR hCursor = NULL;
    HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(nID), pszResouceType);
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

    return hCursor;
}

BEGIN_INTERFACE_LIST(CGEKEngine)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKContextObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSystemObserver)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKGameApplication)
    INTERFACE_LIST_ENTRY_COM(IGEKEngine)
    INTERFACE_LIST_ENTRY_COM(IGEKInputManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKEngine)

CGEKEngine::CGEKEngine(void)
    : m_hLogFile(nullptr)
    , m_bWindowActive(false)
    , m_bSendInput(false)
    , m_hCursorPointer(nullptr)
{
    DeleteFile(L"log.xml");
    m_hLogFile = CreateFile(L"log.xml", GENERIC_ALL, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (m_hLogFile != nullptr && m_hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD nNumWritten = 0;
        CStringA strMessage("<?xml version=\"1.0\"?>\r\n<logging>\r\n");
        WriteFile(m_hLogFile, strMessage.GetString(), strMessage.GetLength(), &nNumWritten, nullptr);
    }
}

CGEKEngine::~CGEKEngine(void)
{
    if (m_hCursorPointer)
    {
        DestroyIcon(m_hCursorPointer);
    }

    if (m_hLogFile != nullptr && m_hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD nNumWritten = 0;
        CStringA strMessage("</logging>\r\n");
        WriteFile(m_hLogFile, strMessage.GetString(), strMessage.GetLength(), &nNumWritten, nullptr);
        CloseHandle(m_hLogFile);
    }
}

HRESULT CGEKEngine::Load(LPCWSTR pName)
{
    GEKFUNCTION(L"Name(%s)", pName);
    HRESULT hRetVal = m_spPopulationManager->Load(pName);
    if (FAILED(hRetVal))
    {
        m_spPopulationManager->Free();
    }
    else
    {
        m_bSendInput = true;
        m_nTotalTime = 0.0;
        m_nTimeAccumulator = 0.0f;
        m_kTimer.Reset();
    }

    return hRetVal;
}

void CGEKEngine::CheckInput(UINT32 nKey, const GEKVALUE &kValue)
{
    if (nKey == VK_ESCAPE && !kValue.GetBoolean())
    {
        m_bSendInput = !m_bSendInput;
        m_kTimer.Pause(!m_bWindowActive || !m_bSendInput);
    }
    else if (m_bSendInput)
    {
        auto pIterator = m_aInputBindings.find(nKey);
        if (pIterator != m_aInputBindings.end())
        {
            OnCommand((*pIterator).second, nullptr, 0);
            CGEKObservable::SendEvent(TGEKEvent<IGEKInputObserver>(std::bind(&IGEKInputObserver::OnAction, std::placeholders::_1, (*pIterator).second, kValue)));
        }
    }
}

STDMETHODIMP CGEKEngine::Initialize(void)
{
    HRESULT hRetVal = GetContext()->AddCachedClass(CLSID_GEKEngine, GetUnknown());
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(GetContext(), (IGEKContextObserver *)GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        GEKFUNCTION(nullptr);

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
                CStringW strID = CA2W(pID, CP_UTF8);
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
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetContext()->AddCachedObserver(CLSID_GEKVideoSystem, (IGEK3DVideoObserver *)GetUnknown());
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetContext()->CreateInstance(CLSID_GEKPopulationSystem, IID_PPV_ARGS(&m_spPopulationManager));
        }
        
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderSystem, IID_PPV_ARGS(&m_spRenderManager));
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
    if (m_spPopulationManager)
    {
        m_spPopulationManager->FreeSystems();
    }

    m_spRenderManager = nullptr;
    m_spPopulationManager = nullptr;
    GetContext()->RemoveCachedObserver(CLSID_GEKVideoSystem, (IGEK3DVideoObserver *)GetUnknown());
    CGEKObservable::RemoveObserver(m_spSystem, (IGEKSystemObserver *)this);
    CGEKObservable::RemoveObserver(GetContext(), (IGEKContextObserver *)this);
    m_spSystem = nullptr;
    GetContext()->RemoveCachedClass(CLSID_GEKEngine);
    xmlCleanupParser();
}

STDMETHODIMP_(void) CGEKEngine::OnLog(LPCSTR pFile, UINT32 nLine, GEKLOGTYPE eType, LPCWSTR pMessage)
{
    if (m_hLogFile != nullptr && m_hLogFile != INVALID_HANDLE_VALUE)
    {
        CPathA kFile(pFile);
        kFile.StripPath();

        CStringA strFile = kFile.m_strPath;

        CStringA strMessage;
        if (eType == GEK_LOGSTART)
        {
            strMessage.Format("<%s line=\"%d\">%S\r\n", strFile.GetString(), nLine, pMessage);
        }
        else if (eType == GEK_LOGEND)
        {
            strMessage.Format("%S</%s>\r\n", pMessage, strFile.GetString());
        }
        else
        {
            strMessage.Format("<%s line=\"%d\">%S</%s>\r\n", strFile.GetString(), nLine, pMessage, strFile.GetString());
        }

        DWORD nNumWritten = 0;
        WriteFile(m_hLogFile, strMessage.GetString(), strMessage.GetLength(), &nNumWritten, nullptr);
    }
}

STDMETHODIMP_(void) CGEKEngine::OnEvent(UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
    switch (nMessage)
    {
    case WM_SETCURSOR:
        SetCursor(m_hCursorPointer);
        nResult = 1;
        break;

    case WM_ACTIVATE:
        if (HIWORD(wParam))
        {
            m_bWindowActive = false;
            m_kTimer.Pause(!m_bWindowActive || !m_bSendInput);
        }
        else
        {
            switch (LOWORD(wParam))
            {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
                m_bWindowActive = true;
                m_kTimer.Pause(!m_bWindowActive || !m_bSendInput);
                break;

            case WA_INACTIVE:
                m_bWindowActive = false;
                m_kTimer.Pause(!m_bWindowActive || !m_bSendInput);
                break;
            };
        }

        nResult = 1;
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
        if (m_bSendInput)
        {
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
            m_kTimer.Update();
            m_nTimeAccumulator += m_kTimer.GetUpdateTime();
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

STDMETHODIMP_(void) CGEKEngine::OnPreReset(void)
{
}

STDMETHODIMP CGEKEngine::OnPostReset(void)
{
    return S_OK;
}

STDMETHODIMP_(void) CGEKEngine::Run(void)
{
    REQUIRE_VOID_RETURN(m_spSystem);
    m_spSystem->Run();
}

STDMETHODIMP_(void) CGEKEngine::OnCommand(LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams)
{
    if (_wcsicmp(pCommand, L"quit") == 0)
    {
        m_spSystem->Stop();
    }
    else if (_wcsicmp(pCommand, L"newgame") == 0)
    {
        Load(L"demo");
    }
    else if (_wcsicmp(pCommand, L"setresolution") == 0)
    {
        if (nNumParams == 3 && pParams)
        {
            m_spSystem->GetConfig().SetValue(L"video", L"xsize", pParams[0]);
            m_spSystem->GetConfig().SetValue(L"video", L"ysize", pParams[1]);
            m_spSystem->GetConfig().SetValue(L"video", L"windowed", pParams[2]);
            if (FAILED(m_spSystem->Reset()))
            {
                m_spSystem->Stop();
            }
        }
    }
}
