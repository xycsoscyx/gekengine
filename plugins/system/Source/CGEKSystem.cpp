#include "CGEKSystem.h"
#include "IGEKInputSystem.h"
#include "IGEKAudioSystem.h"
#include "IGEKVideoSystem.h"
#include <time.h>

#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKObservable)
    INTERFACE_LIST_ENTRY_COM(IGEKContextUser)
    INTERFACE_LIST_ENTRY_COM(IGEKContextObserver)
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
    CGEKObservable::RemoveObserver(GetContext(), this);
}

STDMETHODIMP CGEKSystem::OnRegistration(IUnknown *pObject)
{
    CComQIPtr<IGEKSystemUser> spSystemUser(pObject);
    if (spSystemUser != nullptr)
    {
        return spSystemUser->Register(this);
    }

    return S_OK;
}

STDMETHODIMP CGEKSystem::Initialize(void)
{
    HRESULT hRetVal = CoInitialize(0);
    if (SUCCEEDED(hRetVal))
    {
        hRetVal = CGEKObservable::AddObserver(GetContext(), this);
    }

    if (SUCCEEDED(hRetVal)) 
    {
        m_kConfig.Load(L"%root%\\config.cfg");
        m_nXSize = StrToUINT32(m_kConfig.GetValue(L"video", L"xsize", L"640"));
        m_nYSize = StrToUINT32(m_kConfig.GetValue(L"video", L"ysize", L"480"));
        m_bIsWindowed = StrToBoolean(m_kConfig.GetValue(L"video", L"windowed", L"1"));

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
        m_hWindow = CreateWindow(L"GEKvX_Engine_314159", L"GEKvX Engine", 
                                    WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, 
                                    iCenterX, iCenterY, iWinX, iWinY, 
                                    0, nullptr, GetModuleHandle(nullptr), 0);
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
        if (m_spVideoSystem != nullptr)
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
        hRetVal = GetContext()->CreateInstance(CLSID_GEKInputSystem, IID_PPV_ARGS(&m_spInputSystem));
    }

    if (SUCCEEDED(hRetVal))
    {
        ShowWindow(m_hWindow, SW_SHOW);
        UpdateWindow(m_hWindow);
    }

    return hRetVal;
}

STDMETHODIMP CGEKSystem::Reset(void)
{
    m_nXSize = StrToUINT32(m_kConfig.GetValue(L"video", L"xsize", L"640"));
    m_nYSize = StrToUINT32(m_kConfig.GetValue(L"video", L"ysize", L"480"));
    m_bIsWindowed = StrToBoolean(m_kConfig.GetValue(L"video", L"windowed", L"1"));
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

STDMETHODIMP_(UINT32) CGEKSystem::ParseValue(LPCWSTR pValue)
{
    REQUIRE_RETURN(pValue, 0);

    UINT32 nValue = 0;
    if (pValue[0] == L'%')
    {
        if (pValue[wcslen(pValue) - 1] == L'%')
        {
            CStringW strValue = pValue;
            strValue.Trim(L'%');

            int nPosition = 0;
            CStringW strGroup = strValue.Tokenize(L".", nPosition);
            CStringW strName = strValue.Tokenize(L".", nPosition);
            nValue = StrToUINT32(GetConfig().GetValue(strGroup, strName, L"0"));
        }
    }
    else
    {
        nValue = StrToUINT32(pValue);
    }

    return nValue;
}

STDMETHODIMP_(bool) CGEKSystem::IsRunning(void)
{
    return m_bIsRunning;
}

STDMETHODIMP_(void) CGEKSystem::Run(void)
{
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