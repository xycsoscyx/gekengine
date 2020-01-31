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
        GEK_CONTEXT_USER(Window, Gek::Window::Description)
            , public Gek::Window
        {
        private:
            HWND window = nullptr;
            HCURSOR cursorList[8];
            uint16_t highSurrogate = 0;

        public:
            Window(Context *context, Window::Description description)
                : ContextRegistration(context)
                , cursorList{
                nullptr,
                LoadCursor(nullptr, IDC_ARROW),
                LoadCursor(nullptr, IDC_IBEAM),
                LoadCursor(nullptr, IDC_HAND),
                LoadCursor(nullptr, IDC_SIZENS),
                LoadCursor(nullptr, IDC_SIZEWE),
                LoadCursor(nullptr, IDC_SIZENESW),
                LoadCursor(nullptr, IDC_SIZENWSE),
                }
            {
                WNDCLASSEX windowClass;
                windowClass.cbSize = sizeof(windowClass);
                windowClass.style = CS_HREDRAW | CS_VREDRAW | (description.hasOwnContext ? CS_OWNDC : 0);
                windowClass.lpfnWndProc = [](HWND handle, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
                {
                    Window *window = reinterpret_cast<Window *>(GetWindowLongPtr(handle, GWLP_USERDATA));
                    if (window)
                    {
                        switch (message)
                        {
                        case WM_SETCURSOR:
                            switch (LOWORD(lParam))
                            {
                            case HTCLIENT:
                                if (true)
                                {
                                    Cursor cursor = Cursor::None;
                                    window->onSetCursor(cursor);

                                    auto windowsCursor = window->cursorList[static_cast<uint8_t>(cursor)];
                                    ShowCursor(windowsCursor ? true : false);
                                    SetCursor(windowsCursor);
                                    return TRUE;
                                }
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
                            window->onKeyPressed(wParam, true);
                            break;

                        case WM_KEYUP:
                            window->onKeyPressed(wParam, false);
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
                auto wideDescription(String::Widen(description.className));
                windowClass.lpszClassName = wideDescription.data();
                ATOM classAtom = RegisterClassEx(&windowClass);
                if (!classAtom)
                {
                    throw std::exception("Unable to register window class");
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
                window = CreateWindow(wideDescription.data(), String::Widen(description.windowName).data(), windowFlags, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
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
