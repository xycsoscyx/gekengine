#include "GEK/Render/Window.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <atlbase.h>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
#endif

#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
#endif

namespace Gek
{
    namespace Win32
    {
        const unsigned char KeyToNative[256] =
        {
            255,255,255,255, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
            77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 49, 50,
            51, 52, 53, 54, 55, 56, 57, 48, 13, 27,  8,  9, 32,189,187,219,
            221,220,255,186,222,192,188,190,191, 20,112,113,114,115,116,117,
            118,119,120,121,122,123, 44,145, 19, 45, 36, 33, 46, 35, 34, 39,
            37, 40, 38,144,111,106,109,107,255, 97, 98, 99,100,101,102,103,
            104,105, 96,110,255,255,255,255,124,125,126,127,128,129,130,131,
            132,133,134,135,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            17, 16, 18,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        };

        const unsigned char NativeToKey[256] =
        {
            255,255,255,255,255,255,255,255, 42, 43,255,255,255, 40,255,255,
            225,224,226, 72, 57,255,255,255,255,255,255, 41,255,255,255,255,
            44, 75, 78, 77, 74, 80, 82, 79, 81,255,255,255, 70, 73, 76,255,
            39, 30, 31, 32, 33, 34, 35, 36, 37, 38,255,255,255,255,255,255,
            255,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
            19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,255,255,255,255,255,
            98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 85, 87,255, 86, 99, 84,
            58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,104,105,106,107,
            108,109,110,111,112,113,114,115,255,255,255,255,255,255,255,255,
            83, 71,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255, 51, 46, 54, 45, 55, 56,
            53,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255, 47, 49, 48, 52,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
            255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
        };

        GEK_CONTEXT_USER(Window, Gek::Window::Description)
            , public Gek::Window
        {
        private:
            HWND window = nullptr;

        public:
            Window(Context *context, Window::Description description)
                : ContextRegistration(context)
            {
                WNDCLASS windowClass;
                windowClass.style = CS_HREDRAW | CS_VREDRAW | (description.hasOwnContext ? CS_OWNDC : 0);
                windowClass.lpfnWndProc = [](HWND handle, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
                {
                    Window *window = reinterpret_cast<Window *>(GetWindowLongPtr(handle, GWLP_USERDATA));
                    if (window)
                    {
                        switch (message)
                        {
                        case WM_SETCURSOR:
                            if (LOWORD(lParam) == HTCLIENT)
                            {
                                bool showCursor = true;
                                window->onSetCursor(showCursor);
                                ShowCursor(showCursor);
                                return TRUE;
                            }
                            else
                            {
                                ShowCursor(true);
                            }

                            break;

                        case WM_LBUTTONDOWN:
                            window->onMouseClicked(Window::Button::Left, true);
                            break;

                        case WM_LBUTTONUP:
                            window->onMouseClicked(Window::Button::Left, false);
                            break;

                        case WM_RBUTTONDOWN:
                            window->onMouseClicked(Window::Button::Right, true);
                            break;

                        case WM_RBUTTONUP:
                            window->onMouseClicked(Window::Button::Right, false);
                            break;

                        case WM_MBUTTONDOWN:
                            window->onMouseClicked(Window::Button::Middle, true);
                            break;

                        case WM_MBUTTONUP:
                            window->onMouseClicked(Window::Button::Middle, false);
                            break;

                        case WM_MOUSEWHEEL:
                            window->onMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
                            break;

                        case WM_MOUSEMOVE:
                            window->onMousePosition((int16_t)(lParam), (int16_t)(lParam >> 16));
                            break;

                        case WM_KEYDOWN:
                            if (wParam >= 0 && wParam < 256)
                            {
                                window->onKeyPressed(static_cast<Key>(NativeToKey[wParam]), true);
                            }

                            break;

                        case WM_KEYUP:
                            if (wParam >= 0 && wParam < 256)
                            {
                                window->onKeyPressed(static_cast<Key>(NativeToKey[wParam]), false);
                            }

                            break;

                        case WM_CHAR:
                            if ((HIWORD(lParam) & KF_REPEAT) == 0)
                            {
                                window->onCharacter(wParam);
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

                                    RAWINPUT *rawInput = reinterpret_cast<RAWINPUT *>(rawInputBuffer.data());
                                    if (rawInput->header.dwType == RIM_TYPEMOUSE)
                                    {
                                        window->onMouseMovement(rawInput->data.mouse.lLastX, rawInput->data.mouse.lLastY);
                                    }
                                }
                            }

                            break;

                        case WM_CLOSE:
                            window->onClose();
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
                        };
                    }

                    return DefWindowProc(handle, message, wParam, lParam);
                };

                windowClass.cbClsExtra = 0;
                windowClass.cbWndExtra = 0;
                windowClass.hInstance = GetModuleHandle(nullptr);
                windowClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
                windowClass.hCursor = LoadCursor(GetModuleHandle(nullptr), IDC_ARROW);
                windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
                windowClass.lpszMenuName = nullptr;
				auto wideClassName(String::Widen(description.className));
				windowClass.lpszClassName = wideClassName.c_str();
                ATOM classAtom = RegisterClass(&windowClass);
                if (!classAtom)
                {
                    throw std::exception("Unable to register window class");
                }

                auto windowFlags = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX);

                RECT clientRectangle;
                clientRectangle.left = 0;
                clientRectangle.top = 0;
                clientRectangle.right = 1;
                clientRectangle.bottom = 1;
                AdjustWindowRect(&clientRectangle, WS_OVERLAPPEDWINDOW, false);
                int windowWidth = (clientRectangle.right - clientRectangle.left);
                int windowHeight = (clientRectangle.bottom - clientRectangle.top);
                window = CreateWindow(wideClassName.c_str(), String::Widen(description.windowName).c_str(), windowFlags, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
                if (window == nullptr)
                {
                    throw std::exception("Unable to create window");
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
            void readEvents(void)
            {
                MSG message = { 0 };
                while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                };
            }

            void *getBaseWindow(void) const
            {
                return window;
            }

            Math::Int4 getClientRectangle(void) const
            {
                Math::Int4 rectangle;
                GetClientRect(window, (RECT *)&rectangle);
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
