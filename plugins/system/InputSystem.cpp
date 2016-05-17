#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\InputSystem.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
    static BOOL CALLBACK setDeviceAxisInfo(LPCDIDEVICEOBJECTINSTANCE deviceObjectInstance, void *userData)
    {
        LPDIRECTINPUTDEVICE7 device = (LPDIRECTINPUTDEVICE7)userData;

        DIPROPRANGE propertyRange = { 0 };
        propertyRange.diph.dwSize = sizeof(DIPROPRANGE);
        propertyRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        propertyRange.diph.dwObj = deviceObjectInstance->dwOfs;
        propertyRange.diph.dwHow = DIPH_BYOFFSET;
        propertyRange.lMin = -1000;
        propertyRange.lMax = +1000;
        device->SetProperty(DIPROP_RANGE, &propertyRange.diph);

        DIPROPDWORD propertyDeadZone = { 0 };
        propertyDeadZone.diph.dwSize = sizeof(DIPROPDWORD);
        propertyDeadZone.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        propertyDeadZone.diph.dwObj = deviceObjectInstance->dwOfs;
        propertyDeadZone.diph.dwHow = DIPH_BYOFFSET;
        propertyDeadZone.dwData = 1000;
        device->SetProperty(DIPROP_DEADZONE, &propertyDeadZone.diph);

        return DIENUM_CONTINUE;
    }

    class DeviceImplementation : public UnknownMixin
        , public InputDevice
    {
    protected:
        CComPtr<IDirectInputDevice8> device;
        UINT32 buttonCount;

        std::vector<UINT8> buttonStateList;

        Math::Float3 axisValues;
        Math::Float3 rotationValues;
        float pointOfView;

    public:
        DeviceImplementation(void)
            : buttonCount(0)
            , pointOfView(0.0f)
        {
        }

        virtual ~DeviceImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(DeviceImplementation)
            INTERFACE_LIST_ENTRY_COM(InputDevice)
        END_INTERFACE_LIST_UNKNOWN

        STDMETHODIMP_(UINT32) getButtonCount(void) const
        {
            return buttonCount;
        }

        STDMETHODIMP_(UINT8) getButtonState(UINT32 buttonIndex) const
        {
            return buttonStateList[buttonIndex];
        }

        STDMETHODIMP_(Math::Float3) getAxis(void) const
        {
            return axisValues;
        }

        STDMETHODIMP_(Math::Float3) getRotation(void) const
        {
            return rotationValues;
        }

        STDMETHODIMP_(float) getPointOfView(void) const
        {
            return pointOfView;
        }
    };

    class KeyboardImplementation : public DeviceImplementation
    {
    public:
        KeyboardImplementation(void)
        {
            buttonStateList.resize(256);
        }

        HRESULT initialize(IDirectInput8 *directInput, HWND window)
        {
            HRESULT resultValue = directInput->CreateDevice(GUID_SysKeyboard, &device, nullptr);
            if (SUCCEEDED(resultValue))
            {
                resultValue = device->SetDataFormat(&c_dfDIKeyboard);
                if (SUCCEEDED(resultValue))
                {
                    DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                    resultValue = device->SetCooperativeLevel(window, flags);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = device->Acquire();
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP update(void)
        {
            HRESULT resultValue = E_FAIL;

            UINT32 retryCount = 5;
            unsigned char rawKeyBuffer[256] = { 0 };
            do
            {
                resultValue = device->GetDeviceState(sizeof(rawKeyBuffer), (void *)&rawKeyBuffer);
                if (FAILED(resultValue))
                {
                    resultValue = device->Acquire();
                }
            } while ((resultValue == DIERR_INPUTLOST) && (retryCount-- > 0));

            if (SUCCEEDED(resultValue))
            {
                for (UINT32 keyIndex = 0; keyIndex < 256; keyIndex++)
                {
                    if (rawKeyBuffer[keyIndex] & 0x80 ? true : false)
                    {
                        if (buttonStateList[keyIndex] & Input::State::None)
                        {
                            buttonStateList[keyIndex] = (Input::State::Down | Input::State::Pressed);
                        }
                        else
                        {
                            buttonStateList[keyIndex] = Input::State::Down;
                        }
                    }
                    else
                    {
                        if (buttonStateList[keyIndex] & Input::State::Down)
                        {
                            buttonStateList[keyIndex] = (Input::State::None | Input::State::Released);
                        }
                        else
                        {
                            buttonStateList[keyIndex] = Input::State::None;
                        }
                    }
                }
            }

            return resultValue;
        }
    };

    class MouseImplementation : public DeviceImplementation
    {
    public:
        MouseImplementation(void)
        {
        }

        ~MouseImplementation(void)
        {
        }

        HRESULT initialize(IDirectInput8 *directInput, HWND window)
        {
            HRESULT resultValue = directInput->CreateDevice(GUID_SysMouse, &device, nullptr);
            if (SUCCEEDED(resultValue))
            {
                resultValue = device->SetDataFormat(&c_dfDIMouse2);
                if (SUCCEEDED(resultValue))
                {
                    DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                    resultValue = device->SetCooperativeLevel(window, flags);
                    if (SUCCEEDED(resultValue))
                    {
                        DIDEVCAPS deviceCaps = { 0 };
                        deviceCaps.dwSize = sizeof(DIDEVCAPS);
                        resultValue = device->GetCapabilities(&deviceCaps);
                        if (SUCCEEDED(resultValue))
                        {
                            device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);

                            buttonCount = deviceCaps.dwButtons;
                            buttonStateList.resize(buttonCount);

                            resultValue = device->Acquire();
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP update(void)
        {
            HRESULT resultValue = S_OK;

            UINT32 retryCount = 5;
            DIMOUSESTATE2 mouseStates;
            do
            {
                resultValue = device->GetDeviceState(sizeof(DIMOUSESTATE2), (void *)&mouseStates);
                if (FAILED(resultValue))
                {
                    resultValue = device->Acquire();
                }
            } while ((resultValue == DIERR_INPUTLOST) && (retryCount-- > 0));

            if (SUCCEEDED(resultValue))
            {
                axisValues.x = float(mouseStates.lX);
                axisValues.y = float(mouseStates.lY);
                axisValues.z = float(mouseStates.lZ);
                for (UINT32 buttonIndex = 0; buttonIndex < getButtonCount(); buttonIndex++)
                {
                    if (mouseStates.rgbButtons[buttonIndex] & 0x80 ? true : false)
                    {
                        if (buttonStateList[buttonIndex] & Input::State::None)
                        {
                            buttonStateList[buttonIndex] = (Input::State::Down | Input::State::Pressed);
                        }
                        else
                        {
                            buttonStateList[buttonIndex] = Input::State::Down;
                        }
                    }
                    else
                    {
                        if (buttonStateList[buttonIndex] & Input::State::Down)
                        {
                            buttonStateList[buttonIndex] = (Input::State::None | Input::State::Released);
                        }
                        else
                        {
                            buttonStateList[buttonIndex] = Input::State::None;
                        }
                    }
                }
            }

            return resultValue;
        }
    };

    class JoystickImplementation : public DeviceImplementation
    {
    public:
        JoystickImplementation(void)
        {
        }

        ~JoystickImplementation(void)
        {
        }

        HRESULT initialize(IDirectInput8 *directInput, HWND window, const GUID &deviceID)
        {
            HRESULT resultValue = directInput->CreateDevice(deviceID, &device, nullptr);
            if (SUCCEEDED(resultValue))
            {
                resultValue = device->SetDataFormat(&c_dfDIJoystick2);
                if (SUCCEEDED(resultValue))
                {
                    DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                    resultValue = device->SetCooperativeLevel(window, flags);
                    if (SUCCEEDED(resultValue))
                    {
                        DIDEVCAPS deviceCaps = { 0 };
                        deviceCaps.dwSize = sizeof(DIDEVCAPS);
                        resultValue = device->GetCapabilities(&deviceCaps);
                        if (SUCCEEDED(resultValue))
                        {
                            device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);

                            buttonCount = deviceCaps.dwButtons;
                            buttonStateList.resize(buttonCount);

                            resultValue = device->Acquire();
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP update(void)
        {
            HRESULT resultValue = S_OK;

            UINT32 retryCount = 5;
            DIJOYSTATE2 joystickStates;
            do
            {
                resultValue = device->Poll();
                if (SUCCEEDED(resultValue))
                {
                    resultValue = device->GetDeviceState(sizeof(DIJOYSTATE2), (void *)&joystickStates);
                }

                if (FAILED(resultValue))
                {
                    resultValue = device->Acquire();
                }
            } while ((resultValue == DIERR_INPUTLOST) && (retryCount-- > 0));

            if (SUCCEEDED(resultValue))
            {
                if (LOWORD(joystickStates.rgdwPOV[0]) == 0xFFFF)
                {
                    pointOfView = -1.0f;
                }
                else
                {
                    pointOfView = (float(joystickStates.rgdwPOV[0]) / 100.0f);
                }

                axisValues.x = float(joystickStates.lX);
                axisValues.y = float(joystickStates.lY);
                axisValues.z = float(joystickStates.lZ);
                rotationValues.x = float(joystickStates.lRx);
                rotationValues.y = float(joystickStates.lRy);
                rotationValues.z = float(joystickStates.lRz);
                for (UINT32 buttonIndex = 0; buttonIndex < getButtonCount(); buttonIndex++)
                {
                    if (joystickStates.rgbButtons[buttonIndex] & 0x80 ? true : false)
                    {
                        if (buttonStateList[buttonIndex] & Input::State::None)
                        {
                            buttonStateList[buttonIndex] = (Input::State::Down | Input::State::Pressed);
                        }
                        else
                        {
                            buttonStateList[buttonIndex] = Input::State::Down;
                        }
                    }
                    else
                    {
                        if (buttonStateList[buttonIndex] & Input::State::Down)
                        {
                            buttonStateList[buttonIndex] = (Input::State::None | Input::State::Released);
                        }
                        else
                        {
                            buttonStateList[buttonIndex] = Input::State::None;
                        }
                    }
                }
            }

            return resultValue;
        }
    };

    class InputSystemImplementation : public ContextUserMixin
        , public InputSystem
    {
    private:
        HWND window;
        CComPtr<IDirectInput8> directInput;
        CComPtr<InputDevice> mouseDevice;
        CComPtr<InputDevice> keyboardDevice;
        std::vector<CComPtr<InputDevice>> joystickDeviceList;

    private:
        static BOOL CALLBACK joystickEnumeration(LPCDIDEVICEINSTANCE deviceObjectInstance, void *userData)
        {
            InputSystemImplementation *inputSystem = (InputSystemImplementation *)userData;
            if (inputSystem)
            {
                inputSystem->addJoystick(deviceObjectInstance);
            }

            return DIENUM_CONTINUE;
        }

        void addJoystick(LPCDIDEVICEINSTANCE deviceObjectInstance)
        {
            HRESULT resultValue = E_FAIL;
            CComPtr<JoystickImplementation> joystick = new JoystickImplementation();
            if (joystick != nullptr)
            {
                resultValue = joystick->initialize(directInput, window, deviceObjectInstance->guidInstance);
                if (SUCCEEDED(resultValue))
                {
                    CComPtr<InputDevice> joystickDevice;
                    resultValue = joystickDevice->QueryInterface(IID_PPV_ARGS(&joystickDevice));
                    if (SUCCEEDED(resultValue) && joystickDevice != nullptr)
                    {
                        joystickDeviceList.push_back(joystickDevice);
                    }
                }
            }
        }

    public:
        InputSystemImplementation(void)
            : window(nullptr)
        {
        }

        ~InputSystemImplementation(void)
        {
            joystickDeviceList.clear();
        }

        BEGIN_INTERFACE_LIST(InputSystemImplementation)
            INTERFACE_LIST_ENTRY_COM(InputSystem)
        END_INTERFACE_LIST_USER

        // Interface
        STDMETHODIMP initialize(HWND window)
        {
            GEK_REQUIRE(window);

            HRESULT resultValue = E_FAIL;
            this->window = window;
            resultValue = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID FAR *)&directInput, nullptr);
            if (SUCCEEDED(resultValue) && directInput != nullptr)
            {
                resultValue = E_OUTOFMEMORY;
                CComPtr<KeyboardImplementation> keyboard = new KeyboardImplementation();
                if (keyboard != nullptr)
                {
                    resultValue = keyboard->initialize(directInput, window);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = keyboard->QueryInterface(IID_PPV_ARGS(&keyboardDevice));
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<MouseImplementation> mouse = new MouseImplementation();
                    if (mouse != nullptr)
                    {
                        resultValue = mouse->initialize(directInput, window);
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = mouse->QueryInterface(IID_PPV_ARGS(&mouseDevice));
                        }
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, joystickEnumeration, LPVOID(this), DIEDFL_ATTACHEDONLY);
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(InputDevice *) getKeyboard(void)
        {
            GEK_REQUIRE(keyboardDevice);
            return keyboardDevice;
        }

        STDMETHODIMP_(InputDevice *) getMouse(void)
        {
            GEK_REQUIRE(mouseDevice);
            return mouseDevice;
        }

        STDMETHODIMP_(UINT32) getJoystickCount(void)
        {
            return joystickDeviceList.size();
        }

        STDMETHODIMP_(InputDevice *) getJoystick(UINT32 deviceIndex)
        {
            if (deviceIndex < joystickDeviceList.size())
            {
                return joystickDeviceList[deviceIndex];
            }

            return nullptr;
        }

        STDMETHODIMP update(void)
        {
            GEK_REQUIRE(keyboardDevice);
            GEK_REQUIRE(mouseDevice);

            mouseDevice->update();
            keyboardDevice->update();
            for (auto &joystickDevice : joystickDeviceList)
            {
                joystickDevice->update();
            }

            return S_OK;
        }
    };

    REGISTER_CLASS(InputSystemImplementation);
}; // namespace Gek
