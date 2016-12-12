#include "GEK/System/Window.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <atlbase.h>

namespace Gek
{
    namespace Win32
    {
        GEK_CONTEXT_USER(Window)
            , public Gek::Window
        {
        private:
            HWND window = nullptr;

        public:
            Window(Context *context)
                : ContextRegistration(context)
            {
                WNDCLASS windowClass;
                windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
                windowClass.lpfnWndProc = [](HWND handle, uint32_t message, WPARAM wParam, LPARAM lParam) -> LRESULT
                {
                    Gek::Window *window = reinterpret_cast<Gek::Window *>(GetWindowLongPtr(handle, GWLP_USERDATA));
                    if (window)
                    {
                        Gek::Window::Event eventData(message, wParam, lParam);
                        auto resultValue = window->onEvent(eventData);
                        if (resultValue.handled)
                        {
                            return resultValue.result;
                        }
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
                windowClass.lpszClassName = L"GEKvX_Engine_Demo";
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
                window = CreateWindow(windowClass.lpszClassName, L"GEKvX Application - Demo", windowFlags, CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
                if (window == nullptr)
                {
                    throw std::exception("Unable to create window");
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
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace Win32
}; // namespace Gek
