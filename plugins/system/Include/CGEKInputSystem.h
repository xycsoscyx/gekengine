#pragma once

#define DIRECTINPUT_VERSION 0x0800

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <dinput.h>

class CGEKInputSystem : public CGEKUnknown
                      , public IGEKInputSystem
{
private:
    HWND m_hWindow;
    CComPtr<IDirectInput8> m_spDirectInput;
    CComPtr<IGEKInputDevice> m_spMouse;
    CComPtr<IGEKInputDevice> m_spKeyboard;
    std::vector<CComPtr<IGEKInputDevice>> m_aJoysticks;

private:
    static BOOL CALLBACK JoyStickEnum(LPCDIDEVICEINSTANCE pkInstance, void *pContext);
    void AddJoystick(LPCDIDEVICEINSTANCE pkInstance);

public:
    CGEKInputSystem(void);
    virtual ~CGEKInputSystem(void);
    DECLARE_UNKNOWN(CGEKInputSystem);

    // IGEKInputSystem
    STDMETHOD(Initialize)                       (THIS_ HWND hWindow);
    STDMETHOD_(IGEKInputDevice *, GetKeyboard)  (THIS);
    STDMETHOD_(IGEKInputDevice *, GetMouse)     (THIS);
    STDMETHOD_(UINT32, GetNumJoysticks)         (THIS);
    STDMETHOD_(IGEKInputDevice *, GetJoystick)  (THIS_ UINT32 nDevice);
    STDMETHOD(Refresh)                          (THIS);
};
