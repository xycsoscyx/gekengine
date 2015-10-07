#include "GEK\Context\UserMixin.h"
#include "GEK\System\InputInterface.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
    namespace Input
    {
        namespace Device
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

            class Base : public Context::User::Mixin
                , public Interface
            {
            protected:
                CComPtr<IDirectInputDevice8> device;
                UINT32 buttonCount;

                std::vector<UINT8> buttonStateList;

                Math::Float3 axisValues;
                Math::Float3 rotationValues;
                float pointOfView;

            public:
                Base(void)
                    : buttonCount(0)
                    , pointOfView(0.0f)
                {
                }

                virtual ~Base(void)
                {
                }

                BEGIN_INTERFACE_LIST(Base)
                    INTERFACE_LIST_ENTRY_COM(Input::Device::Interface)
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

            class Keyboard : public Base
            {
            public:
                Keyboard(void)
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
                            UINT32 flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
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
                                if (buttonStateList[keyIndex] & State::None)
                                {
                                    buttonStateList[keyIndex] = (State::Down | State::Pressed);
                                }
                                else
                                {
                                    buttonStateList[keyIndex] = State::Down;
                                }
                            }
                            else
                            {
                                if (buttonStateList[keyIndex] & State::Down)
                                {
                                    buttonStateList[keyIndex] = (State::None | State::Released);
                                }
                                else
                                {
                                    buttonStateList[keyIndex] = State::None;
                                }
                            }
                        }
                    }

                    return resultValue;
                }
            };

            class Mouse : public Base
            {
            public:
                Mouse(void)
                {
                }

                ~Mouse(void)
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
                            UINT32 flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
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
                                if (buttonStateList[buttonIndex] & State::None)
                                {
                                    buttonStateList[buttonIndex] = (State::Down | State::Pressed);
                                }
                                else
                                {
                                    buttonStateList[buttonIndex] = State::Down;
                                }
                            }
                            else
                            {
                                if (buttonStateList[buttonIndex] & State::Down)
                                {
                                    buttonStateList[buttonIndex] = (State::None | State::Released);
                                }
                                else
                                {
                                    buttonStateList[buttonIndex] = State::None;
                                }
                            }
                        }
                    }

                    return resultValue;
                }
            };

            class Joystick : public Base
            {
            public:
                Joystick(void)
                {
                }

                ~Joystick(void)
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
                            UINT32 flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
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
                        resultValue = device->GetDeviceState(sizeof(DIJOYSTATE2), (void *)&joystickStates);
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
                                if (buttonStateList[buttonIndex] & State::None)
                                {
                                    buttonStateList[buttonIndex] = (State::Down | State::Pressed);
                                }
                                else
                                {
                                    buttonStateList[buttonIndex] = State::Down;
                                }
                            }
                            else
                            {
                                if (buttonStateList[buttonIndex] & State::Down)
                                {
                                    buttonStateList[buttonIndex] = (State::None | State::Released);
                                }
                                else
                                {
                                    buttonStateList[buttonIndex] = State::None;
                                }
                            }
                        }
                    }

                    return resultValue;
                }
            };
        }; // namespace Device

        class System : public Context::User::Mixin
                    , public Input::Interface
        {
        private:
            HWND window;
            CComPtr<IDirectInput8> directInput;
            CComPtr<Device::Interface> mouseDevice;
            CComPtr<Device::Interface> keyboardDevice;
            std::vector<CComPtr<Device::Interface>> joystickDeviceList;

        private:
            static BOOL CALLBACK joystickEnumeration(LPCDIDEVICEINSTANCE deviceObjectInstance, void *userData)
            {
                System *inputSystem = (System *)userData;
                if (inputSystem)
                {
                    inputSystem->addJoystick(deviceObjectInstance);
                }

                return DIENUM_CONTINUE;
            }

            void addJoystick(LPCDIDEVICEINSTANCE deviceObjectInstance)
            {
                gekLogScope(__FUNCTION__);

                CComPtr<Device::Joystick> joystick = new Device::Joystick();
                if (joystick != nullptr)
                {
                    if (SUCCEEDED(gekCheckResult(joystick->initialize(directInput, window, deviceObjectInstance->guidInstance))))
                    {
                        CComPtr<Device::Interface> joystickDevice;
                        joystickDevice->QueryInterface(IID_PPV_ARGS(&joystickDevice));
                        if (joystickDevice != nullptr)
                        {
                            joystickDeviceList.push_back(joystickDevice);
                        }
                    }
                }
            }

        public:
            System(void)
                : window(nullptr)
            {
            }

            ~System(void)
            {
                joystickDeviceList.clear();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(Input::Interface)
            END_INTERFACE_LIST_USER

            // Interface
            STDMETHODIMP initialize(HWND window)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("0x%p", window);

                this->window = window;
                HRESULT resultValue = E_FAIL;
                gekCheckResult(resultValue = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID FAR *)&directInput, nullptr));
                if (directInput != nullptr)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Device::Keyboard> keyboard = new Device::Keyboard();
                    if (keyboard != nullptr)
                    {
                        if (SUCCEEDED(gekCheckResult(resultValue = keyboard->initialize(directInput, window))))
                        {
                            resultValue = keyboard->QueryInterface(IID_PPV_ARGS(&keyboardDevice));
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<Device::Mouse> mouse = new Device::Mouse();
                        if (mouse != nullptr)
                        {
                            if (SUCCEEDED(gekCheckResult(resultValue = mouse->initialize(directInput, window))))
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

            STDMETHODIMP_(Device::Interface *) getKeyboard(void)
            {
                REQUIRE_RETURN(keyboardDevice, nullptr);
                return keyboardDevice;
            }

            STDMETHODIMP_(Device::Interface *) getMouse(void)
            {
                REQUIRE_RETURN(mouseDevice, nullptr);
                return mouseDevice;
            }

            STDMETHODIMP_(UINT32) getJoystickCount(void)
            {
                return joystickDeviceList.size();
            }

            STDMETHODIMP_(Device::Interface *) getJoystick(UINT32 deviceIndex)
            {
                if (deviceIndex < joystickDeviceList.size())
                {
                    return joystickDeviceList[deviceIndex];
                }

                return nullptr;
            }

            STDMETHODIMP update(void)
            {
                REQUIRE_RETURN(keyboardDevice, E_INVALIDARG);
                REQUIRE_RETURN(mouseDevice, E_INVALIDARG);

                mouseDevice->update();
                keyboardDevice->update();
                for (auto &spDevice : joystickDeviceList)
                {
                    spDevice->update();
                }

                return S_OK;
            }
        };

        REGISTER_CLASS(System);
    }; // namespace Input
}; // namespace Gek
