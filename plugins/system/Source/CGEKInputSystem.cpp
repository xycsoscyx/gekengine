#include "CGEKInputSystem.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

static BOOL CALLBACK SetDeviceAxisInfo(LPCDIDEVICEOBJECTINSTANCE pkInstance, void *pContext)
{
    LPDIRECTINPUTDEVICE7 pDevice = (LPDIRECTINPUTDEVICE7)pContext;

    DIPROPRANGE kRange = { 0 };
    kRange.diph.dwSize = sizeof(DIPROPRANGE);
    kRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    kRange.diph.dwObj = pkInstance->dwOfs;
    kRange.diph.dwHow = DIPH_BYOFFSET;
    kRange.lMin = -1000;
    kRange.lMax = +1000;
    pDevice->SetProperty(DIPROP_RANGE, &kRange.diph);

    DIPROPDWORD kDeadZone = { 0 };
    kDeadZone.diph.dwSize = sizeof(DIPROPDWORD);
    kDeadZone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    kDeadZone.diph.dwObj = pkInstance->dwOfs;
    kDeadZone.diph.dwHow = DIPH_BYOFFSET;
    kDeadZone.dwData = 1000;
    pDevice->SetProperty(DIPROP_DEADZONE, &kDeadZone.diph);

    return DIENUM_CONTINUE;
}

class CGEKInputDevice : public CGEKUnknown
                      , public IGEKInputDevice
{
protected:
    CComPtr<IDirectInputDevice8> m_spDevice;
    UINT32 m_nNumButtons;
    UINT32 m_nNumAxis;

    std::vector<UINT32> m_aStates;

    float3 m_nAxis;
    float3 m_nRotation;
    float m_nPOV;

public:
    DECLARE_UNKNOWN(CGEKInputDevice);
    CGEKInputDevice(void)
        : m_nNumButtons(0)
        , m_nNumAxis(0)
        , m_nPOV(0.0f)
    {
    }

    virtual ~CGEKInputDevice(void)
    {
    }

    STDMETHODIMP_(UINT32) GetNumButtons(void) const
    {
        return m_nNumButtons;
    }

    STDMETHODIMP_(UINT32) GetState(UINT32 nIndex) const
    {
        return m_aStates[nIndex];
    }

    STDMETHODIMP_(UINT32) GetNumAxis(void) const
    {
        return m_nNumAxis; 
    }
    
    STDMETHODIMP_(float3) GetAxis(void) const
    {
        return m_nAxis;
    }

    STDMETHODIMP_(float3) GetRotation(void) const
    {
        return m_nRotation;
    }

    STDMETHODIMP_(float) GetPOV(void) const
    {
        return m_nPOV;
    }
};

BEGIN_INTERFACE_LIST(CGEKInputDevice)
    INTERFACE_LIST_ENTRY_COM(IGEKInputDevice)
END_INTERFACE_LIST_UNKNOWN

class CGEKKeyboard : public CGEKInputDevice
{
public:
    CGEKKeyboard(void)
    {
        m_aStates.resize(256);
    }

    ~CGEKKeyboard(void)
    {
    }

    HRESULT Initialize(IDirectInput8 *pDirectInput, HWND hWindow)
    {
        HRESULT hRetVal = pDirectInput->CreateDevice(GUID_SysKeyboard, &m_spDevice, nullptr);
        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_spDevice->SetDataFormat(&c_dfDIKeyboard);
            if (SUCCEEDED(hRetVal)) 
            {
                UINT32 iFlags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                hRetVal = m_spDevice->SetCooperativeLevel(hWindow, iFlags);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = m_spDevice->Acquire();
                }
            }
        }

        return hRetVal;
    }

    STDMETHODIMP Refresh(void)
    {
        HRESULT hRetVal = E_FAIL;

        UINT32 nCount = 5;
        unsigned char aBuffer[256] = { 0 };
        do
        {
            hRetVal = m_spDevice->GetDeviceState(sizeof(aBuffer), (void *)&aBuffer); 
            if (FAILED(hRetVal))
            {
                hRetVal = m_spDevice->Acquire();
            }
        } while ((hRetVal == DIERR_INPUTLOST)&&(nCount-- > 0));

        if (SUCCEEDED(hRetVal))
        {
            for(UINT32 nIndex = 0; nIndex < 256; nIndex++) 
            {
                if (aBuffer[nIndex] & 0x80 ? true : false)
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::NONE)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::DOWN | GEKINPUT::STATE::PRESSED);
                    }
                    else
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::DOWN;
                    }
                }
                else
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::DOWN)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::NONE | GEKINPUT::STATE::RELEASED);
                    }
                    else 
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::NONE;
                    }
                }
            }
        }

        return hRetVal;
    }
};

class CGEKMouse : public CGEKInputDevice
{
public:
    CGEKMouse(void)
    {
    }

    ~CGEKMouse(void)
    {
    }

    HRESULT Initialize(IDirectInput8 *pDirectInput, HWND hWindow)
    {
        HRESULT hRetVal = pDirectInput->CreateDevice(GUID_SysMouse, &m_spDevice, nullptr);
        if (SUCCEEDED(hRetVal)) 
        {
            hRetVal = m_spDevice->SetDataFormat(&c_dfDIMouse2);
            if (SUCCEEDED(hRetVal)) 
            {
                UINT32 iFlags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                hRetVal = m_spDevice->SetCooperativeLevel(hWindow, iFlags);
                if (SUCCEEDED(hRetVal)) 
                {
                    DIDEVCAPS kCaps = { 0 };
                    kCaps.dwSize = sizeof(DIDEVCAPS);
                    hRetVal = m_spDevice->GetCapabilities(&kCaps);
                    if (SUCCEEDED(hRetVal)) 
                    {
                        m_spDevice->EnumObjects(SetDeviceAxisInfo, (void *)m_spDevice, DIDFT_AXIS);

                        m_nNumAxis = kCaps.dwAxes;
                        m_nNumButtons = kCaps.dwButtons;
                        m_aStates.resize(m_nNumButtons);

                        DIDEVICEINSTANCE kInstance;
                        kInstance.dwSize = sizeof(DIDEVICEINSTANCE);
                        m_spDevice->GetDeviceInfo(&kInstance);

                        hRetVal = m_spDevice->Acquire();
                    }
                }
            }
        }

        return hRetVal;
    }

    STDMETHODIMP Refresh(void)
    {
        HRESULT hRetVal = S_OK;

        UINT32 nCount = 5;
        DIMOUSESTATE2 kStates;
        do
        {
            hRetVal = m_spDevice->GetDeviceState(sizeof(DIMOUSESTATE2), (void *)&kStates); 
            if (FAILED(hRetVal))
            {
                hRetVal = m_spDevice->Acquire();
            }
        } while ((hRetVal == DIERR_INPUTLOST)&&(nCount-- > 0));

        if (SUCCEEDED(hRetVal))
        {
            m_nAxis.x = float(kStates.lX);
            m_nAxis.y = float(kStates.lY);
            m_nAxis.z = float(kStates.lZ);
            for(UINT32 nIndex = 0; nIndex < GetNumButtons(); nIndex++)
            {
                if (kStates.rgbButtons[nIndex] & 0x80 ? true : false)
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::NONE)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::DOWN | GEKINPUT::STATE::PRESSED);
                    }
                    else
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::DOWN;
                    }
                }
                else
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::DOWN)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::NONE | GEKINPUT::STATE::RELEASED);
                    }
                    else 
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::NONE;
                    }
                }
            }
        }

        return hRetVal;
    }
};

class CGEKJoystick : public CGEKInputDevice
{
private:
    GUID m_kGUID;
    
public:
    CGEKJoystick(void)
    {
    }

    ~CGEKJoystick(void)
    {
    }

    HRESULT Initialize(IDirectInput8 *pDirectInput, HWND hWindow, const GUID &kGUID)
    {
        HRESULT hRetVal = pDirectInput->CreateDevice(m_kGUID, &m_spDevice, nullptr);
        if (SUCCEEDED(hRetVal)) 
        {
            hRetVal = m_spDevice->SetDataFormat(&c_dfDIJoystick2);
            if (SUCCEEDED(hRetVal)) 
            {
                UINT32 iFlags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                hRetVal = m_spDevice->SetCooperativeLevel(hWindow, iFlags);
                if (SUCCEEDED(hRetVal)) 
                {
                    DIDEVCAPS kCaps = { 0 };
                    kCaps.dwSize = sizeof(DIDEVCAPS);
                    hRetVal = m_spDevice->GetCapabilities(&kCaps);
                    if (SUCCEEDED(hRetVal)) 
                    {
                        m_spDevice->EnumObjects(SetDeviceAxisInfo, (void *)m_spDevice, DIDFT_AXIS);

                        m_nNumAxis = kCaps.dwAxes;
                        m_nNumButtons = kCaps.dwButtons;
                        m_aStates.resize(m_nNumButtons);

                        DIDEVICEINSTANCE kInstance;
                        kInstance.dwSize = sizeof(DIDEVICEINSTANCE);
                        m_spDevice->GetDeviceInfo(&kInstance);

                        hRetVal = m_spDevice->Acquire();
                    }
                }
            }
        }

        return hRetVal;
    }

    STDMETHODIMP Refresh(void)
    {
        HRESULT hRetVal = S_OK;

        UINT32 nCount = 5;
        DIJOYSTATE2 kStates;
        do
        {
            hRetVal = m_spDevice->Poll();
            hRetVal = m_spDevice->GetDeviceState(sizeof(DIJOYSTATE2), (void *)&kStates); 
            if (FAILED(hRetVal))
            {
                hRetVal = m_spDevice->Acquire();
            }
        } while ((hRetVal == DIERR_INPUTLOST)&&(nCount-- > 0));

        if (SUCCEEDED(hRetVal))
        {
            if (LOWORD(kStates.rgdwPOV[0]) == 0xFFFF)
            {
                m_nPOV = -1.0f;
            }
            else
            {
                m_nPOV = (float(kStates.rgdwPOV[0]) / 100.0f);
            }

            m_nAxis.x = float(kStates.lX);
            m_nAxis.y = float(kStates.lY);
            m_nAxis.z = float(kStates.lZ);
            m_nRotation.x = float(kStates.lRx);
            m_nRotation.y = float(kStates.lRy);
            m_nRotation.z = float(kStates.lRz);
            for(UINT32 nIndex = 0; nIndex < GetNumButtons(); nIndex++)
            {
                if (kStates.rgbButtons[nIndex] & 0x80 ? true : false)
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::NONE)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::DOWN | GEKINPUT::STATE::PRESSED);
                    }
                    else
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::DOWN;
                    }
                }
                else
                {
                    if (m_aStates[nIndex] & GEKINPUT::STATE::DOWN)
                    {
                        m_aStates[nIndex] = (GEKINPUT::STATE::NONE | GEKINPUT::STATE::RELEASED);
                    }
                    else 
                    {
                        m_aStates[nIndex] = GEKINPUT::STATE::NONE;
                    }
                }
            }
        }

        return hRetVal;
    }
};

BOOL CALLBACK CGEKInputSystem::JoyStickEnum(LPCDIDEVICEINSTANCE pkInstance, void *pContext)
{
    CGEKInputSystem *pSystem = (CGEKInputSystem *)pContext;
    if (pSystem)
    {
        pSystem->AddJoystick(pkInstance);
    }

    return DIENUM_CONTINUE;
}

void CGEKInputSystem::AddJoystick(LPCDIDEVICEINSTANCE pkInstance)
{
    CComPtr<CGEKJoystick> spJoystickDevice = new CGEKJoystick();
    if (spJoystickDevice != nullptr)
    {
        if (SUCCEEDED(spJoystickDevice->Initialize(m_spDirectInput, m_hWindow, pkInstance->guidInstance)))
        {
            CComPtr<IGEKInputDevice> spInputDevice;
            spJoystickDevice->QueryInterface(IID_PPV_ARGS(&spInputDevice));
            if (spInputDevice != nullptr)
            {
                m_aJoysticks.push_back(spInputDevice);
            }
        }
    }
}

BEGIN_INTERFACE_LIST(CGEKInputSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKInputSystem)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKInputSystem);

CGEKInputSystem::CGEKInputSystem(void)
    : m_hWindow(nullptr)
{
}

CGEKInputSystem::~CGEKInputSystem(void)
{
    m_aJoysticks.clear();
}

HRESULT CGEKInputSystem::Initialize(HWND hWindow)
{
    m_hWindow = hWindow;
    HRESULT hRetVal = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID FAR *)&m_spDirectInput, nullptr);
    if (m_spDirectInput != nullptr)
    {
        hRetVal = E_OUTOFMEMORY;
        CComPtr<CGEKKeyboard> spKeyboard = new CGEKKeyboard();
        if (spKeyboard != nullptr)
        {
            hRetVal = spKeyboard->Initialize(m_spDirectInput, hWindow);
            if (SUCCEEDED(hRetVal))
            {
                hRetVal = spKeyboard->QueryInterface(IID_PPV_ARGS(&m_spKeyboard));
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = E_OUTOFMEMORY;
            CComPtr<CGEKMouse> spMouse = new CGEKMouse();
            if (spMouse != nullptr)
            {
                hRetVal = spMouse->Initialize(m_spDirectInput, hWindow);
                if (SUCCEEDED(hRetVal))
                {
                    hRetVal = spMouse->QueryInterface(IID_PPV_ARGS(&m_spMouse));
                }
            }
        }

        if (SUCCEEDED(hRetVal))
        {
            hRetVal = m_spDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, JoyStickEnum, LPVOID(this), DIEDFL_ATTACHEDONLY);
        }
    }

    return hRetVal;
}

IGEKInputDevice *CGEKInputSystem::GetKeyboard(void)
{
    REQUIRE_RETURN(m_spKeyboard, nullptr);
    return m_spKeyboard;
}

IGEKInputDevice *CGEKInputSystem::GetMouse(void)
{
    REQUIRE_RETURN(m_spMouse, nullptr);
    return m_spMouse;
}

UINT32 CGEKInputSystem::GetNumJoysticks(void)
{
    return m_aJoysticks.size();
}

IGEKInputDevice *CGEKInputSystem::GetJoystick(UINT32 nDevice)
{
    if (nDevice < m_aJoysticks.size())
    {
        return m_aJoysticks[nDevice];
    }

    return nullptr;
}

HRESULT CGEKInputSystem::Refresh(void)
{
    REQUIRE_RETURN(m_spMouse && m_spKeyboard, E_INVALIDARG);

    m_spMouse->Refresh();
    m_spKeyboard->Refresh();
    for (auto &spDevice : m_aJoysticks)
    {
        spDevice->Refresh();
    }

    return S_OK;
}
