#pragma once

#define DIRECTINPUT_VERSION 0x0800

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <dinput.h>

class CGEKInputSystem : public CGEKUnknown
                      , public CGEKContextUser
                      , public CGEKSystemUser
                      , public IGEKContextObserver
                      , public IGEKInputSystem
{
private:
    CComPtr<IDirectInput8> m_spDirectInput;
    CComPtr<IGEKInputDevice> m_spMouse;
    CComPtr<IGEKInputDevice> m_spKeyboard;
    std::vector<CComPtr<IGEKInputDevice>> m_aJoysticks;

private:
    static BOOL CALLBACK JoyStickEnum(LPCDIDEVICEINSTANCE pkInstance, void *pContext);

public:
    CGEKInputSystem(void);
    virtual ~CGEKInputSystem(void);
    DECLARE_UNKNOWN(CGEKInputSystem);

    // IGEKContextObserver
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject);

    // IGEKUnknown
    STDMETHOD(Initialize)                       (THIS);

    // IGEKInputSystem
    STDMETHOD_(IGEKInputDevice *, GetKeyboard)  (THIS);
    STDMETHOD_(IGEKInputDevice *, GetMouse)     (THIS);
    STDMETHOD_(UINT32, GetNumJoysticks)         (THIS);
    STDMETHOD_(IGEKInputDevice *, GetJoystick)  (THIS_ UINT32 nDevice);
    STDMETHOD(Refresh)                          (THIS);
};
