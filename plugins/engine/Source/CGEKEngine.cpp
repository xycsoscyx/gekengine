#include "CGEKEngine.h"
#include "IGEKRenderManager.h"
#include "IGEKPopulationManager.h"
#include "CGEKEntity.h"
#include <libxml/parserInternals.h>
#include <windowsx.h>
#include <algorithm>
#include <atlpath.h>

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

#pragma comment (lib, "awesomium.lib")

BEGIN_INTERFACE_LIST(CGEKEngine)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKContextObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKSystemObserver)
    INTERFACE_LIST_ENTRY_COM(IGEKGameApplication)
    INTERFACE_LIST_ENTRY_COM(IGEKEngine)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKEngine)

CGEKEngine::CGEKEngine(void)
    : m_nTotalTime(0.0)
    , m_nTimeAccumulator(0.0)
    , m_bWindowActive(false)
    , m_bCaptureMouse(false)
{
}

CGEKEngine::~CGEKEngine(void)
{
}

void CGEKEngine::CheckInput(UINT32 nKey, const GEKVALUE &kValue)
{
    auto pIterator = m_aInputBindings.find(nKey);
    if (pIterator != m_aInputBindings.end())
    {
        m_spPopulationManager->OnInputEvent(((*pIterator).second), kValue);
    }
}

STDMETHODIMP CGEKEngine::OnRegistration(IUnknown *pObject)
{
    HRESULT hRetVal = S_OK;
    CComQIPtr<IGEKEngineUser> spuser(pObject);
    if (spuser != nullptr)
    {
        hRetVal = spuser->Register(this);
    }
    
    return hRetVal;
}

STDMETHODIMP CGEKEngine::Initialize(void)
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
    m_aInputBindings[WM_MOUSEWHEEL] = L"height";
    m_aInputBindings[WM_MOUSEMOVE] = L"turn";

    m_aInputBindings[VK_ESCAPE] = L"escape";

    m_bWindowActive = true;
    HRESULT hRetVal = CGEKObservable::AddObserver(GetContext(), (IGEKContextObserver *)this);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKSystem, IID_PPV_ARGS(&m_spSystem));
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(m_spSystem, (IGEKSystemObserver *)this);
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKRenderManager, IID_PPV_ARGS(&m_spRenderManager));
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKPopulationManager, IID_PPV_ARGS(&m_spPopulationManager));
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKEngine::Destroy(void)
{
    m_spRenderManager = nullptr;
    m_spPopulationManager = nullptr;
    CGEKObservable::RemoveObserver(m_spSystem, (IGEKSystemObserver *)this);
    m_spSystem = nullptr;

    CGEKObservable::RemoveObserver(GetContext(), (IGEKContextObserver *)this);

    xmlCleanupParser();
}

STDMETHODIMP CGEKEngine::OnEvent(UINT32 nMessage, WPARAM wParam, LPARAM lParam, LRESULT &nResult)
{
    switch (nMessage)
    {
    case WM_SETCURSOR:
        SetCursor(nullptr);
        nResult = 1;
        break;

    case WM_ACTIVATE:
        if (HIWORD(wParam))
        {
            m_bWindowActive = false;
            m_kTimer.Pause(true);
        }
        else
        {
            switch (LOWORD(wParam))
            {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
                m_bWindowActive = true;
                m_kTimer.Pause(false);
                break;

            case WA_INACTIVE:
                m_bWindowActive = false;
                m_kTimer.Pause(true);
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

    return S_OK;
}

STDMETHODIMP CGEKEngine::OnRun(void)
{
    m_spPopulationManager->LoadScene(L"q3dm1", L"info_player_start_1");
    m_nTotalTime = 0.0;
    m_kTimer.Reset();
    return S_OK;
}

STDMETHODIMP CGEKEngine::OnStop(void)
{
    m_bWindowActive = false;
    m_spRenderManager->FreeWorld();
    m_spPopulationManager->FreeScene();
    return S_OK;
}

STDMETHODIMP CGEKEngine::OnStep(void)
{
    if (m_bWindowActive)
    {
        if (m_bCaptureMouse)
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
                CheckInput(WM_MOUSEMOVE, float2(float(nCursorMoveX), float(nCursorMoveY)));
            }
        }

        m_kTimer.Update();
        m_nTimeAccumulator += m_kTimer.GetUpdateTime();
        while (m_nTimeAccumulator > (1.0 / 30.0))
        {
            m_nTotalTime += (1.0f / 30.0f);
            m_nTimeAccumulator -= (1.0 / 30.0);
            m_spPopulationManager->Update(float(m_nTotalTime), (1.0f / 30.0f));
        };
     
        if(SUCCEEDED(m_spRenderManager->BeginFrame()))
        {
            m_spPopulationManager->Render();
            m_spRenderManager->EndFrame();
        }
    }

    return S_OK;
}

STDMETHODIMP_(void) CGEKEngine::Run(void)
{
    REQUIRE_VOID_RETURN(m_spSystem);
    m_spSystem->Run();
}

STDMETHODIMP_(void) CGEKEngine::CaptureMouse(bool bCapture)
{
    m_bCaptureMouse = bCapture;
}

STDMETHODIMP_(void) CGEKEngine::OnCommand(LPCWSTR pCommand, LPCWSTR *pParams, UINT32 nNumParams)
{
    if (_wcsicmp(pCommand, L"quit") == 0)
    {
        m_spSystem->Stop();
    }
    else if (_wcsicmp(pCommand, L"newgame") == 0)
    {
        m_spPopulationManager->LoadScene(L"demo", L"info_player_start_1");
        m_nTotalTime = 0.0;
        m_kTimer.Reset();
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
