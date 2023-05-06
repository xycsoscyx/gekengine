#include "GEK/System/Window.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <atlbase.h>
#include "resource.h"

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace Gek
{
    HINSTANCE GetDLLInstance(void);

    namespace Win32
    {
        Window::Key ConvertKey(uint32_t key, uint32_t state)
        {
        case VK_A:      return Window::Key::A;
        case VK_B:      return Window::Key::B;
        case VK_C:      return Window::Key::C;
        case VK_D:      return Window::Key::D;
        case VK_E:      return Window::Key::E;
        case VK_F:      return Window::Key::F;
        case VK_G:      return Window::Key::G;
        case VK_H:      return Window::Key::H;
        case VK_I:      return Window::Key::I;
        case VK_J:      return Window::Key::J;
        case VK_K:      return Window::Key::K;
        case VK_L:      return Window::Key::L;
        case VK_M:      return Window::Key::M;
        case VK_N:      return Window::Key::N;
        case VK_O:      return Window::Key::O;
        case VK_P:      return Window::Key::P;
        case VK_Q:      return Window::Key::Q;
        case VK_R:      return Window::Key::R;
        case VK_S:      return Window::Key::S;
        case VK_T:      return Window::Key::T;
        case VK_U:      return Window::Key::U;
        case VK_V:      return Window::Key::V;
        case VK_W:      return Window::Key::W;
        case VK_X:      return Window::Key::X;
        case VK_Y:      return Window::Key::Y;
        case VK_Z:      return Window::Key::Z;
        case VK_1:      return Window::Key::Key1;
        case VK_2:      return Window::Key::Key2;
        case VK_3:      return Window::Key::Key3;
        case VK_4:      return Window::Key::Key4;
        case VK_5:      return Window::Key::Key5;
        case VK_6:      return Window::Key::Key6;
        case VK_7:      return Window::Key::Key7;
        case VK_8:      return Window::Key::Key8;
        case VK_9:      return Window::Key::Key9;
        case VK_0:      return Window::Key::Key0;
        case VK_RETURN:      return Window::Key::Return;
        case VK_ESC:      return Window::Key::Escape;
        case VK_:      return Window::Key::Backspace;
        case VK_:      return Window::Key::Tab;
        case VK_:      return Window::Key::Space;
        case VK_:      return Window::Key::Minus;
        case VK_:      return Window::Key::Equals;
        case VK_:      return Window::Key::LeftBracket;
        case VK_:      return Window::Key::RightBracket;
        case VK_:      return Window::Key::Backslash;
        case VK_:      return Window::Key::Semicolon;
        case VK_:      return Window::Key::Quote;
        case VK_:      return Window::Key::Grave;
        case VK_:      return Window::Key::Comma;
        case VK_:      return Window::Key::Period;
        case VK_:      return Window::Key::Slash;
        case VK_:      return Window::Key::CapsLock;
        case VK_:      return Window::Key::F1;
        case VK_:      return Window::Key::F2;
        case VK_:      return Window::Key::F3;
        case VK_:      return Window::Key::F4;
        case VK_:      return Window::Key::F5;
        case VK_:      return Window::Key::F6;
        case VK_:      return Window::Key::F7;
        case VK_:      return Window::Key::F8;
        case VK_:      return Window::Key::F9;
        case VK_:      return Window::Key::F10;
        case VK_:      return Window::Key::F11;
        case VK_:      return Window::Key::F12;
        case VK_:      return Window::Key::PrintScreen;
        case VK_:      return Window::Key::ScrollLock;
        case VK_:      return Window::Key::Pause;
        case VK_:      return Window::Key::Insert;
        case VK_:      return Window::Key::Home;
        case VK_:      return Window::Key::PageUp;
        case VK_:      return Window::Key::Delete;
        case VK_:      return Window::Key::End;
        case VK_:      return Window::Key::PageDown;
        case VK_:      return Window::Key::Right;
        case VK_:      return Window::Key::Left;
        case VK_:      return Window::Key::Down;
        case VK_:      return Window::Key::Up;
        case VK_:      return Window::Key::KeyPadNumLock;
        case VK_:      return Window::Key::KeyPadDivide;
        case VK_:      return Window::Key::KeyPadMultiply;
        case VK_:      return Window::Key::KeyPadSubtract;
        case VK_:      return Window::Key::KeyPadAdd;
        case VK_:      return Window::Key::KeyPadEnter;
        case VK_:      return Window::Key::KeyPad1;
        case VK_:      return Window::Key::KeyPad2;
        case VK_:      return Window::Key::KeyPad3;
        case VK_:      return Window::Key::KeyPad4;
        case VK_:      return Window::Key::KeyPad5;
        case VK_:      return Window::Key::KeyPad6;
        case VK_:      return Window::Key::KeyPad7;
        case VK_:      return Window::Key::KeyPad8;
        case VK_:      return Window::Key::KeyPad9;
        case VK_:      return Window::Key::KeyPad0;
        case VK_:      return Window::Key::KeyPadPoint;
        case VK_:      return Window::Key::NonUSBackslash;
        case VK_:      return Window::Key::KeyPadEquals;
        case VK_:      return Window::Key::F13;
        case VK_:      return Window::Key::F14;
        case VK_:      return Window::Key::F15;
        case VK_:      return Window::Key::F16;
        case VK_:      return Window::Key::F17;
        case VK_:      return Window::Key::F18;
        case VK_:      return Window::Key::F19;
        case VK_:      return Window::Key::F20;
        case VK_:      return Window::Key::F21;
        case VK_:      return Window::Key::F22;
        case VK_:      return Window::Key::F23;
        case VK_:      return Window::Key::F24;
        case VK_:      return Window::Key::Help;
        case VK_:      return Window::Key::Menu;
        case VK_:      return Window::Key::LeftControl;
        case VK_:      return Window::Key::LeftShift;
        case VK_:      return Window::Key::LeftAlt;
        case VK_:      return Window::Key::LeftGUI;
        case VK_:      return Window::Key::RightControl;
        case VK_:      return Window::Key::RightShift;
        case VK_:      return Window::Key::RightAlt;
        case VK_:      return Window::Key::RightGUI;
        };

        GEK_CONTEXT_USER_BASE(Window)
            , public Gek::Window
        {
        private:
            HWND window = nullptr;
            uint16_t highSurrogate = 0;
            std::atomic_bool stop = false;

        public:
            Window(Context *context)
                : ContextRegistration(context)
            {
            }

            ~Window(void)
            {
                if (window)
                {
                    SetWindowLongPtr(window, GWLP_USERDATA, 0);
                    DestroyWindow(window);
                }
            }

            // Window
            void create(Description const& description)
            {
                WNDCLASSEX windowClass;
                windowClass.cbSize = sizeof(windowClass);
                windowClass.style = CS_HREDRAW | CS_VREDRAW | (description.hasOwnContext ? CS_OWNDC : 0);
                windowClass.lpfnWndProc = [](HWND handle, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
                {
                    Window* window = reinterpret_cast<Window*>(GetWindowLongPtr(handle, GWLP_USERDATA));
                    if (window)
                    {
                        switch (message)
                        {
                        case WM_SETCURSOR:
                            switch (LOWORD(lParam))
                            {
                            case HTCLIENT:
                                break;
                            };

                            break;

                        case WM_LBUTTONDOWN:
                        case WM_LBUTTONDBLCLK:
                            window->onMouseClicked(Window::Button::Left, true);
                            break;

                        case WM_LBUTTONUP:
                            window->onMouseClicked(Window::Button::Left, false);
                            break;

                        case WM_RBUTTONDOWN:
                        case WM_RBUTTONDBLCLK:
                            window->onMouseClicked(Window::Button::Right, true);
                            break;

                        case WM_RBUTTONUP:
                            window->onMouseClicked(Window::Button::Right, false);
                            break;

                        case WM_MBUTTONDOWN:
                        case WM_MBUTTONDBLCLK:
                            window->onMouseClicked(Window::Button::Middle, true);
                            break;

                        case WM_MBUTTONUP:
                            window->onMouseClicked(Window::Button::Middle, false);
                            break;

                        case WM_XBUTTONDOWN:
                        case WM_XBUTTONDBLCLK:
                            window->onMouseClicked(wParam & XBUTTON1 ? Window::Button::Forward : Window::Button::Back, true);
                            return true;

                        case WM_XBUTTONUP:
                            window->onMouseClicked(wParam & XBUTTON1 ? Window::Button::Forward : Window::Button::Back, false);
                            return true;

                        case WM_MOUSEWHEEL:
                            window->onMouseWheel(float(GET_WHEEL_DELTA_WPARAM(wParam)) / float(WHEEL_DELTA));
                            break;

                        case WM_MOUSEMOVE:
                            window->onMousePosition((int16_t)(lParam), (int16_t)(lParam >> 16));
                            break;

                        case WM_KEYDOWN:
                            window->onKeyPressed(ConvertKey(wParam), true);
                            break;

                        case WM_KEYUP:
                            window->onKeyPressed(ConvertKey(wParam), false);
                            break;

                        case WM_CHAR:
                            if (IS_HIGH_SURROGATE(wParam))
                            {
                                window->highSurrogate = static_cast<uint16_t>(wParam);
                            }
                            else
                            {
                                if (IS_LOW_SURROGATE(wParam))
                                {
                                    uint16_t lowSurrogate = static_cast<uint16_t>(wParam);
                                    uint32_t character = (window->highSurrogate - HIGH_SURROGATE_START) << 10;
                                    character |= (lowSurrogate - LOW_SURROGATE_START);
                                    character += 0x10000;
                                    window->highSurrogate = 0;
                                    window->onCharacter(character);
                                }
                                else
                                {
                                    uint16_t character = static_cast<uint16_t>(wParam);
                                    window->onCharacter(character);
                                }
                            }

                            break;

                        case WM_INPUT:
                            if (GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT)
                            {
                                uint32_t inputSize = 0;
                                GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &inputSize, sizeof(RAWINPUTHEADER));
                                if (inputSize > 0)
                                {
                                    std::vector<uint8_t> rawInputBuffer(inputSize);
                                    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawInputBuffer.data(), &inputSize, sizeof(RAWINPUTHEADER));

                                    RAWINPUT* rawInput = reinterpret_cast<RAWINPUT*>(rawInputBuffer.data());
                                    if (rawInput->header.dwType == RIM_TYPEMOUSE)
                                    {
                                        window->onMouseMovement(rawInput->data.mouse.lLastX, rawInput->data.mouse.lLastY);
                                    }
                                }
                            }

                            break;

                        case WM_CLOSE:
                            window->onCloseRequested();
                            return TRUE;

                        case WM_ACTIVATE:
                            if (HIWORD(wParam))
                            {
                                window->onActivate(false);
                            }
                            else
                            {
                                switch (LOWORD(wParam))
                                {
                                case WA_ACTIVE:
                                case WA_CLICKACTIVE:
                                    window->onActivate(true);
                                    break;

                                case WA_INACTIVE:
                                    window->onActivate(false);
                                    break;
                                };
                            }

                            return TRUE;

                        case WM_SIZE:
                            window->onSizeChanged(wParam == SIZE_MINIMIZED);
                            break;

                        case WM_SYSCOMMAND:
                            if (wParam == SC_KEYMENU)
                            {
                                /* Remove beeping sound when ALT + some key is pressed. */
                                return FALSE;
                            }

                            break;

                        case WM_SETFOCUS:
                            break;

                        case WM_KILLFOCUS:
                            break;
                        };
                    }

                    return DefWindowProc(handle, message, wParam, lParam);
                };

                windowClass.cbClsExtra = 0;
                windowClass.cbWndExtra = 0;
                windowClass.hInstance = GetModuleHandle(nullptr);
                windowClass.hIcon = LoadIcon(GetDLLInstance(), MAKEINTRESOURCE(IDI_HOURGLASS));
                windowClass.hIconSm = LoadIcon(GetDLLInstance(), MAKEINTRESOURCE(IDI_HOURGLASS));
                windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
                windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                windowClass.lpszMenuName = nullptr;
                windowClass.lpszClassName = description.className.data();
                ATOM classAtom = RegisterClassEx(&windowClass);
                if (!classAtom)
                {
                    throw std::runtime_error("Unable to register window class");
                }

                auto windowFlags = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);
                windowFlags |= (description.allowResize ? WS_THICKFRAME | WS_MAXIMIZEBOX : 0);

                RECT clientRectangle;
                clientRectangle.left = 0;
                clientRectangle.top = 0;
                clientRectangle.right = 1;
                clientRectangle.bottom = 1;
                AdjustWindowRect(&clientRectangle, WS_OVERLAPPEDWINDOW, false);
                int windowWidth = (clientRectangle.right - clientRectangle.left);
                int windowHeight = (clientRectangle.bottom - clientRectangle.top);
                window = CreateWindowA(description.className.data(), description.windowName.data(), windowFlags, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
                if (window == nullptr)
                {
                    throw std::runtime_error("Unable to create window");
                }

                SetWindowLongPtr(window, GWLP_USERDATA, LONG_PTR(this));
                if (description.readMouseMovement)
                {
                    RAWINPUTDEVICE inputDevice;
                    inputDevice.usUsagePage = HID_USAGE_PAGE_GENERIC;
                    inputDevice.usUsage = HID_USAGE_GENERIC_MOUSE;
                    inputDevice.dwFlags = 0;
                    inputDevice.hwndTarget = window;
                    RegisterRawInputDevices(&inputDevice, 1, sizeof(RAWINPUTDEVICE));
                }

                onCreated();
                while (!stop.load())
                {
                    MSG message = { 0 };
                    while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                    {
                        TranslateMessage(&message);
                        DispatchMessage(&message);
                    };

                    onIdle();
                };
            }

            void close(void)
            {
                stop.store(true);
            }

            void *getWindowData(uint32_t data) const
            {
                return window;
            }

            Math::Int4 getClientRectangle(bool moveToScreen = false) const
            {
                Math::Int4 rectangle;
                GetClientRect(window, (RECT *)&rectangle);
                if (moveToScreen)
                {
                    ClientToScreen(window, (POINT *)&rectangle.minimum);
                    ClientToScreen(window, (POINT *)&rectangle.maximum);
                }

                return rectangle;
            }

            Math::Int4 getScreenRectangle(void) const
            {
                Math::Int4 rectangle;
                GetWindowRect(window, (RECT *)&rectangle);
                return rectangle;
            }

            Math::Int2 getCursorPosition(void) const
            {
                Math::Int2 position;
                GetCursorPos((POINT *)&position);
                return position;
            }

            void setCursorPosition(Math::Int2 const &position)
            {
                SetCursorPos(position.x, position.y);
            }

            void setCursorVisibility(bool isVisible)
            {
                ShowCursor(isVisible);
            }

            void setVisibility(bool isVisible)
            {
                ShowWindow(window, (isVisible ? SW_SHOW : SW_HIDE));
                UpdateWindow(window);
            }

            void move(Math::Int2 const &position)
            {
                int32_t x = position.x;
                int32_t y = position.y;
                if (x < 0 || y < 0)
                {
                    RECT clientRectangle;
                    GetWindowRect(window, &clientRectangle);
                    x = (x < 0 ? ((GetSystemMetrics(SM_CXSCREEN) - (clientRectangle.right - clientRectangle.left)) / 2) : x);
                    y = (y < 0 ? ((GetSystemMetrics(SM_CYSCREEN) - (clientRectangle.bottom - clientRectangle.top)) / 2) : y);
                }

                SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace Win32
}; // namespace Gek
