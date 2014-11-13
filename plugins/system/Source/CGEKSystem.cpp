#include "CGEKSystem.h"
#include "IGEK3DVideoSystem.h"
#include "IGEKAudioSystem.h"
#include <time.h>

#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKSystem);

LRESULT CALLBACK CGEKSystem::ManagerBaseProc(HWND hWindow, UINT32 nMessage, WPARAM wParam, LPARAM lParam)
{
    CGEKSystem *pSystem = (CGEKSystem *)GetWindowLong(hWindow, GWL_USERDATA);
    if (pSystem == nullptr) 
    {
        return DefWindowProc(hWindow, nMessage, wParam, lParam);
    }

    return pSystem->ManagerProc(nMessage, wParam, lParam);
}

LRESULT CALLBACK CGEKSystem::ManagerProc(UINT32 nMessage, WPARAM wParam, LPARAM lParam)
{
    if (nMessage == WM_CLOSE)
    {
        m_bIsClosed = true;
        DestroyWindow(m_hWindow);
        return 0;
    }
    else if (nMessage == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else
    {
        LRESULT nResult = -1;
        CGEKObservable::SendEvent(TGEKEvent<IGEKSystemObserver>(std::bind(&IGEKSystemObserver::OnEvent, std::placeholders::_1, nMessage, wParam, lParam, nResult)));
        if (nResult != -1)
        {
            return nResult;
        }
    }

    return DefWindowProc(m_hWindow, nMessage, wParam, lParam);
}

CGEKSystem::CGEKSystem(void)
    : m_hWindow(nullptr)
    , m_bIsClosed(false)
    , m_bIsRunning(false)
    , m_bIsWindowed(false)
    , m_nXSize(0)
    , m_nYSize(0)
{
}

CGEKSystem::~CGEKSystem(void)
{
}

STDMETHODIMP CGEKSystem::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = CoInitialize(0);
    GEKRESULT(SUCCEEDED(hRetVal), L"Call to CoInitialize failed: 0x%08X", hRetVal);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->AddCachedClass(CLSID_GEKSystem, GetUnknown());
    }

    if (SUCCEEDED(hRetVal))
    {
        m_kConfig.Load(L"%root%\\config.cfg");
        m_nXSize = StrToUINT32(m_kConfig.GetValue(L"video", L"xsize", L"640"));
        m_nYSize = StrToUINT32(m_kConfig.GetValue(L"video", L"ysize", L"480"));
        m_bIsWindowed = StrToBoolean(m_kConfig.GetValue(L"video", L"windowed", L"1"));
        GEKLOG(L"Initial Config: %dx%d %s", m_nXSize, m_nYSize, (m_bIsWindowed ? L"Windowed" : L"Fullscreen"));

        WNDCLASSEX kClass = { 0 };
        if (!GetClassInfoEx(GetModuleHandle(nullptr), L"GEKvX_Engine_314159", &kClass))
        {
            WNDCLASS kClass;
            kClass.style                = 0;
            kClass.lpfnWndProc          = ManagerBaseProc;
            kClass.cbClsExtra           = 0;
            kClass.cbWndExtra           = 0;
            kClass.hInstance            = GetModuleHandle(nullptr);
            kClass.hIcon                = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
            kClass.hCursor              = nullptr;
            kClass.hbrBackground        = (HBRUSH)GetStockObject(BLACK_BRUSH);
            kClass.lpszMenuName         = nullptr;
            kClass.lpszClassName        = L"GEKvX_Engine_314159";
            hRetVal = (RegisterClass(&kClass) ? S_OK : E_FAIL);
            GEKRESULT(SUCCEEDED(hRetVal), L"Call to RegisterClass failed: 0x%08X", GetLastError());
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        RECT kRect;
        kRect.left = 0;
        kRect.top = 0;
        kRect.right = m_nXSize;
        kRect.bottom = m_nYSize;
        AdjustWindowRect(&kRect, WS_OVERLAPPEDWINDOW, false);
        int iWinX = (kRect.right - kRect.left);
        int iWinY = (kRect.bottom - kRect.top);
        int iCenterX = 0;
        int iCenterY = 0;

        if (m_bIsWindowed)
        {
            iCenterX = (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((kRect.right - kRect.left) / 2);
            iCenterY = (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((kRect.bottom - kRect.top) / 2);
        }

        hRetVal = E_FAIL;
        m_hWindow = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, 
                                 iCenterX, iCenterY, iWinX, iWinY, 0, nullptr, GetModuleHandle(nullptr), 0);
        GEKRESULT(m_hWindow != nullptr, L"Call to CreateWindow failed: 0x%08X", GetLastError());
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
        if (m_spVideoSystem)
        {
            m_spVideoSystem->SetDefaultTargets();
        }
    }

    if (SUCCEEDED(hRetVal))
    {
        hRetVal = GetContext()->CreateInstance(CLSID_GEKAudioSystem, IID_PPV_ARGS(&m_spAudioSystem));
    }

    if (SUCCEEDED(hRetVal))
    {
        ShowWindow(m_hWindow, SW_SHOW);
        UpdateWindow(m_hWindow);
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKSystem::Destroy(void)
{
    m_spVideoSystem = nullptr;
    m_spAudioSystem = nullptr;
    GetContext()->RemoveCachedClass(CLSID_GEKSystem);
}

STDMETHODIMP CGEKSystem::Reset(void)
{
    GEKFUNCTION(nullptr);
    m_nXSize = StrToUINT32(m_kConfig.GetValue(L"video", L"xsize", L"640"));
    m_nYSize = StrToUINT32(m_kConfig.GetValue(L"video", L"ysize", L"480"));
    m_bIsWindowed = StrToBoolean(m_kConfig.GetValue(L"video", L"windowed", L"1"));
    GEKLOG(L"Reset Config: %dx%d %s", m_nXSize, m_nYSize, (m_bIsWindowed ? L"Windowed" : L"Fullscreen"));

    HRESULT hRetVal = m_spVideoSystem->Reset();
    if (SUCCEEDED(hRetVal) && m_bIsWindowed)
    {
        RECT kRect;
        kRect.left = 0;
        kRect.top = 0;
        kRect.right = m_nXSize;
        kRect.bottom = m_nYSize;
        AdjustWindowRect(&kRect, WS_OVERLAPPEDWINDOW, false);
        int iWinX = (kRect.right - kRect.left);
        int iWinY = (kRect.bottom - kRect.top);
        int iCenterX = 0;
        int iCenterY = 0;

        iCenterX = (GetSystemMetrics(SM_CXFULLSCREEN) / 2) - ((kRect.right - kRect.left) / 2);
        iCenterY = (GetSystemMetrics(SM_CYFULLSCREEN) / 2) - ((kRect.bottom - kRect.top) / 2);
        SetWindowPos(m_hWindow, HWND_TOP, iCenterX, iCenterY, iWinX, iWinY, SWP_NOZORDER);
    }

    return hRetVal;
}

STDMETHODIMP_(CGEKConfig &) CGEKSystem::GetConfig(void)
{
    return m_kConfig;
}

STDMETHODIMP_(HWND) CGEKSystem::GetWindow(void)
{
    return m_hWindow;
}

STDMETHODIMP_(bool) CGEKSystem::IsWindowed(void)
{
    return m_bIsWindowed;
}

STDMETHODIMP_(UINT32) CGEKSystem::GetXSize(void)
{
    return m_nXSize;
}

STDMETHODIMP_(UINT32) CGEKSystem::GetYSize(void)
{
    return m_nYSize;
}

STDMETHODIMP_(CStringW) CGEKSystem::ParseValue(LPCWSTR pValue)
{
    CStringW strValue(pValue);
    while (strValue.Find(L"%") >= 0)
    {
        CStringW strLeft;
        int nPosition = 0;
        if (strValue.GetAt(0) == L'%')
        {
            nPosition = 1;
        }
        else
        {
            strLeft = strValue.Tokenize(L"%", nPosition);
        }

        if (nPosition >= 0)
        {
            CStringW strMiddle = strValue.Tokenize(L"%", nPosition);
            if (nPosition >= 0)
            {
                nPosition = 0;
                CStringW strGroup = strMiddle.Tokenize(L".", nPosition);
                CStringW strName = strMiddle.Tokenize(L".", nPosition);
                if (GetConfig().DoesValueExists(strGroup, strName))
                {
                    strValue.Replace((L"%" + strMiddle + L"%"), GetConfig().GetValue(strGroup, strName, L"0"));
                }
            }
        }
    };

    return strValue;
}

STDMETHODIMP_(bool) CGEKSystem::IsRunning(void)
{
    return m_bIsRunning;
}

STDMETHODIMP_(void) CGEKSystem::Run(void)
{
    GEKFUNCTION(nullptr);
    CGEKObservable::SendEvent(TGEKEvent<IGEKSystemObserver>(std::bind(&IGEKSystemObserver::OnRun, std::placeholders::_1)));

    m_bIsRunning = true;
    MSG kMessage = { 0 };
    while (m_bIsRunning && !m_bIsClosed)
    {
        while (PeekMessage(&kMessage, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&kMessage);
            DispatchMessage(&kMessage);
        };

        CGEKObservable::SendEvent(TGEKEvent<IGEKSystemObserver>(std::bind(&IGEKSystemObserver::OnStep, std::placeholders::_1)));
    };

    CGEKObservable::SendEvent(TGEKEvent<IGEKSystemObserver>(std::bind(&IGEKSystemObserver::OnStop, std::placeholders::_1)));
}

STDMETHODIMP_(void) CGEKSystem::Stop(void)
{
    m_bIsRunning = false;
}
