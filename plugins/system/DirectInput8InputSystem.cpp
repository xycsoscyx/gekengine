#include "GEK\Utility\Exceptions.h"
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
    namespace DirectInput8
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

        class Device
            : public Input::Device
        {
        protected:
            CComPtr<IDirectInputDevice8> device;
            uint32_t buttonCount;

            std::vector<uint8_t> buttonStateList;

            Math::Float3 axisValues;
            Math::Float3 rotationValues;
            float pointOfView;

        public:
            Device(void)
                : buttonCount(0)
                , pointOfView(0.0f)
            {
            }

            virtual ~Device(void)
            {
            }

            // Input::Device
            uint32_t getButtonCount(void) const
            {
                return buttonCount;
            }

            uint8_t getButtonState(uint32_t buttonIndex) const
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

        class Keyboard
            : public Device
        {
        public:
            Keyboard(IDirectInput8 *directInput, HWND window)
            {
                buttonStateList.resize(256);

                HRESULT resultValue = directInput->CreateDevice(GUID_SysKeyboard, &device, nullptr);
                if (FAILED(resultValue))
                {
                    throw Input::CreationFailed();
                }

                resultValue = device->SetDataFormat(&c_dfDIKeyboard);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                resultValue = device->SetCooperativeLevel(window, flags);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                resultValue = device->Acquire();
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }
            }

            // Input::Device
            void poll(void)
            {
                HRESULT resultValue = E_FAIL;

                uint32_t retryCount = 5;
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
                    for (uint32_t keyIndex = 0; keyIndex < 256; keyIndex++)
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

        class Mouse
            : public Device
        {
        public:
            Mouse(IDirectInput8 *directInput, HWND window)
            {
                HRESULT resultValue = directInput->CreateDevice(GUID_SysMouse, &device, nullptr);
                if (FAILED(resultValue))
                {
                    throw Input::CreationFailed();
                }

                resultValue = device->SetDataFormat(&c_dfDIMouse2);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                resultValue = device->SetCooperativeLevel(window, flags);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                DIDEVCAPS deviceCaps = { 0 };
                deviceCaps.dwSize = sizeof(DIDEVCAPS);
                resultValue = device->GetCapabilities(&deviceCaps);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                resultValue = device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                buttonCount = deviceCaps.dwButtons;
                buttonStateList.resize(buttonCount);

                resultValue = device->Acquire();
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }
            }

            // Input::Device
            void poll(void)
            {
                HRESULT resultValue = S_OK;

                uint32_t retryCount = 5;
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
                    for (uint32_t buttonIndex = 0; buttonIndex < getButtonCount(); buttonIndex++)
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

        class Joystick
            : public Device
        {
        public:
            Joystick(IDirectInput8 *directInput, HWND window, const GUID &deviceID)
            {
                HRESULT resultValue = directInput->CreateDevice(deviceID, &device, nullptr);
                if (FAILED(resultValue))
                {
                    throw Input::CreationFailed();
                }

                resultValue = device->SetDataFormat(&c_dfDIJoystick2);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                DWORD flags = DISCL_NONEXCLUSIVE | DISCL_BACKGROUND;
                resultValue = device->SetCooperativeLevel(window, flags);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                DIDEVCAPS deviceCaps = { 0 };
                deviceCaps.dwSize = sizeof(DIDEVCAPS);
                resultValue = device->GetCapabilities(&deviceCaps);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                resultValue = device->EnumObjects(setDeviceAxisInfo, (void *)device, DIDFT_AXIS);
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }

                buttonCount = deviceCaps.dwButtons;
                buttonStateList.resize(buttonCount);

                resultValue = device->Acquire();
                if (FAILED(resultValue))
                {
                    throw Input::InitializationFailed();
                }
            }

            // Input::Device
            void poll(void)
            {
                HRESULT resultValue = S_OK;

                uint32_t retryCount = 5;
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
                    for (uint32_t buttonIndex = 0; buttonIndex < getButtonCount(); buttonIndex++)
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

        GEK_CONTEXT_USER(System, HWND)
            , public Input::System
        {
        private:
            HWND window;
            CComPtr<IDirectInput8> directInput;
            Input::DevicePtr mouseDevice;
            Input::DevicePtr keyboardDevice;
            std::vector<Input::DevicePtr> joystickDeviceList;

        private:
            static BOOL CALLBACK joystickEnumeration(LPCDIDEVICEINSTANCE deviceObjectInstance, void *userData)
            {
                GEK_REQUIRE(deviceObjectInstance);
                GEK_REQUIRE(userData);

                System *inputSystem = static_cast<System *>(userData);
                inputSystem->addJoystick(deviceObjectInstance);

                return DIENUM_CONTINUE;
            }

            void addJoystick(LPCDIDEVICEINSTANCE deviceObjectInstance)
            {
                try
                {
                    Input::DevicePtr joystick(std::make_shared<Joystick>(directInput, window, deviceObjectInstance->guidInstance));
                    joystickDeviceList.push_back(joystick);
                }
                catch (...)
                {
                };
            }

        public:
            System(Context *context, HWND window)
                : ContextRegistration(context)
                , window(window)
            {
                GEK_REQUIRE(window);

                this->window = window;

                HRESULT resultValue = DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (LPVOID FAR *)&directInput, nullptr);
                if (FAILED(resultValue))
                {
                    throw Input::CreationFailed();
                }

                keyboardDevice = std::make_shared<Keyboard>(directInput, window);
                mouseDevice = std::make_shared<Mouse>(directInput, window);
                directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, joystickEnumeration, LPVOID(this), DIEDFL_ATTACHEDONLY);
            }

            ~System(void)
            {
                joystickDeviceList.clear();
            }

            // Input::System
            Input::Device * const getKeyboard(void)
            {
                GEK_REQUIRE(keyboardDevice);
                return keyboardDevice.get();
            }

            Input::Device * const getMouse(void)
            {
                GEK_REQUIRE(mouseDevice);
                return mouseDevice.get();
            }

            uint32_t getJoystickCount(void)
            {
                return joystickDeviceList.size();
            }

            Input::Device * const getJoystick(uint32_t deviceIndex)
            {
                if (deviceIndex < joystickDeviceList.size())
                {
                    return joystickDeviceList[deviceIndex].get();
                }

                return nullptr;
            }

            void pollAllDevices(void)
            {
                GEK_REQUIRE(keyboardDevice);
                GEK_REQUIRE(mouseDevice);

                mouseDevice->poll();
                keyboardDevice->poll();
                for (auto &joystickDevice : joystickDeviceList)
                {
                    joystickDevice->poll();
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(System);
    }; // namespace DirectInput8
}; // namespace Gek
