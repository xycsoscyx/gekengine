#include "GEK\Utility\Trace.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\InputSystem.h"
#include <atlbase.h>
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

    class DeviceImplementation
        : public InputDevice
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

        UINT32 getButtonCount(void) const
        {
            return buttonCount;
        }

        UINT8 getButtonState(UINT32 buttonIndex) const
        {
            return buttonStateList[buttonIndex];
        }

        Math::Float3 getAxis(void) const
        {
            return axisValues;
        }

        Math::Float3 getRotation(void) const
        {
            return rotationValues;
        }

        float getPointOfView(void) const
        {
            return pointOfView;
        }
    };

    class KeyboardImplementation
        : public DeviceImplementation
    {
    public:
        KeyboardImplementation(IDirectInput8 *directInput, HWND window)
        {
            buttonStateList.resize(256);

            HRESULT resultValue = directInput->CreateDevice(GUID_SysKeyboard, &device, nullptr);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to create keyboard device: %v", resultValue);

            resultValue = device->SetDataFormat(&c_dfDIKeyboard);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to set keyboard data format: %v", resultValue);

            DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
            resultValue = device->SetCooperativeLevel(window, flags);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to set keyboard cooperative level: %v", resultValue);

            resultValue = device->Acquire();
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to acquire keyboard device: %v", resultValue);
        }

        void update(void)
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
        }
    };

    class MouseImplementation
        : public DeviceImplementation
    {
    public:
        MouseImplementation(IDirectInput8 *directInput, HWND window)
        {
            HRESULT resultValue = directInput->CreateDevice(GUID_SysMouse, &device, nullptr);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to create mouse device: %v", resultValue);

            resultValue = device->SetDataFormat(&c_dfDIMouse2);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable set mouse data format: %v", resultValue);

            DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
            resultValue = device->SetCooperativeLevel(window, flags);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable set mouse cooperative level: %v", resultValue);

            DIDEVCAPS deviceCaps = { 0 };
            deviceCaps.dwSize = sizeof(DIDEVCAPS);
            resultValue = device->GetCapabilities(&deviceCaps);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable get mouse capabilities: %v", resultValue);

            resultValue = device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable enumerate mouse axis info: %v", resultValue);

            buttonCount = deviceCaps.dwButtons;
            buttonStateList.resize(buttonCount);

            resultValue = device->Acquire();
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable acquire mouse device: %v", resultValue);
        }

        void update(void)
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
        }
    };

    class JoystickImplementation
        : public DeviceImplementation
    {
    public:
        JoystickImplementation(IDirectInput8 *directInput, HWND window, const GUID &deviceID)
        {
            HRESULT resultValue = directInput->CreateDevice(deviceID, &device, nullptr);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to create joystick device: %v", resultValue);

            resultValue = device->SetDataFormat(&c_dfDIJoystick2);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable set joystick data format: %v", resultValue);

            DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
            resultValue = device->SetCooperativeLevel(window, flags);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable set joystick cooperative level: %v", resultValue);

            DIDEVCAPS deviceCaps = { 0 };
            deviceCaps.dwSize = sizeof(DIDEVCAPS);
            resultValue = device->GetCapabilities(&deviceCaps);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable get joystick capabilities: %v", resultValue);

            resultValue = device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable enumerate joystick axis info: %v", resultValue);

            buttonCount = deviceCaps.dwButtons;
            buttonStateList.resize(buttonCount);

            resultValue = device->Acquire();
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable acquire joystick device: %v", resultValue);
        }

        void update(void)
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
        }
    };

    class InputSystemImplementation
        : public ContextRegistration<InputSystemImplementation, HWND>
        , public InputSystem
    {
    private:
        HWND window;
        CComPtr<IDirectInput8> directInput;
        InputDevicePtr mouseDevice;
        InputDevicePtr keyboardDevice;
        std::vector<InputDevicePtr> joystickDeviceList;

    private:
        static BOOL CALLBACK joystickEnumeration(LPCDIDEVICEINSTANCE deviceObjectInstance, void *userData)
        {
            GEK_REQUIRE(deviceObjectInstance);
            GEK_REQUIRE(userData);

            InputSystemImplementation *inputSystem = static_cast<InputSystemImplementation *>(userData);
            inputSystem->addJoystick(deviceObjectInstance);

            return DIENUM_CONTINUE;
        }

        void addJoystick(LPCDIDEVICEINSTANCE deviceObjectInstance)
        {
            try
            {
                InputDevicePtr joystick(makeShared<InputDevice, JoystickImplementation>(directInput, window, deviceObjectInstance->guidInstance));
                joystickDeviceList.push_back(joystick);
            }
            catch (...)
            {
            };
        }

    public:
        InputSystemImplementation(Context *context, HWND window)
            : ContextRegistration(context)
            , window(window)
        {
            GEK_REQUIRE(window);

            this->window = window;

            HRESULT resultValue = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID FAR *)&directInput, nullptr);
            GEK_CHECK_CONDITION(FAILED(resultValue), Input::Exception, "Unable to create DirectInput device: %v", resultValue);

            keyboardDevice = makeShared<InputDevice, KeyboardImplementation>(directInput, window);
            mouseDevice = makeShared<InputDevice, MouseImplementation>(directInput, window);
            directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, joystickEnumeration, LPVOID(this), DIEDFL_ATTACHEDONLY);
        }

        ~InputSystemImplementation(void)
        {
            joystickDeviceList.clear();
        }

        // InputSystem
        InputDevice * const getKeyboard(void)
        {
            GEK_REQUIRE(keyboardDevice);
            return keyboardDevice.get();
        }

        InputDevice * const getMouse(void)
        {
            GEK_REQUIRE(mouseDevice);
            return mouseDevice.get();
        }

        UINT32 getJoystickCount(void)
        {
            return joystickDeviceList.size();
        }

        InputDevice * const getJoystick(UINT32 deviceIndex)
        {
            if (deviceIndex < joystickDeviceList.size())
            {
                return joystickDeviceList[deviceIndex].get();
            }

            return nullptr;
        }

        void update(void)
        {
            GEK_REQUIRE(keyboardDevice);
            GEK_REQUIRE(mouseDevice);

            mouseDevice->update();
            keyboardDevice->update();
            for (auto &joystickDevice : joystickDeviceList)
            {
                joystickDevice->update();
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(InputSystemImplementation);
}; // namespace Gek
