#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/Window.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <future>
#include <thread>
#include <atomic>

namespace Gek
{
    namespace X11
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
        case VK_:      return Window::Key::Key1;
        case VK_:      return Window::Key::Key2;
        case VK_:      return Window::Key::Key3;
        case VK_:      return Window::Key::Key4;
        case VK_:      return Window::Key::Key5;
        case VK_:      return Window::Key::Key6;
        case VK_:      return Window::Key::Key7;
        case VK_:      return Window::Key::Key8;
        case VK_:      return Window::Key::Key9;
        case VK_:      return Window::Key::Key0;
        case VK_:      return Window::Key::Return;
        case VK_:      return Window::Key::Escape;
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
            int screen = 0;
            Display *display = nullptr;
            ::Window window = 0, parentWindow = 0, rootWindow = 0;
            GC graphicContext = nullptr;
            std::atomic_bool stop = false;

            Math::Int4 windowRectangle;

        public:
            Window(Context *context)
                : ContextRegistration(context)
            {
            }

            ~Window(void)
            {
                //XFlush(display);
                XFreeGC(display, graphicContext);
                XDestroyWindow(display, window);
                XCloseDisplay(display);	
            }

            // Window
            void create(Description const &description)
            {
                XInitThreads();

                display = XOpenDisplay(nullptr);
                screen = DefaultScreen(display);
                auto black = BlackPixel(display, screen);
                auto white = WhitePixel(display, screen);

                windowRectangle.set(0, 0, 300, 200);
                window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,	300, 200, 5, white, black);

                uint32_t childrenCount = 0;
                ::Window *childrenWindows = nullptr;
                XQueryTree(display, window, &rootWindow, &parentWindow, &childrenWindows, &childrenCount);

                XSetStandardProperties(display, window, description.windowName.data(), "GEK", None, nullptr, 0, nullptr);

                XSelectInput(display, window, 
                    ExposureMask | 
                    ButtonPressMask | 
                    ButtonReleaseMask | 
                    KeyPressMask | 
                    KeyReleaseMask | 
                    StructureNotifyMask);

                graphicContext = XCreateGC(display, window, 0, 0);        

                XSetBackground(display, graphicContext, white);
                XSetForeground(display, graphicContext, black);

                XClearWindow(display, window);
                XMapRaised(display, window);

                getContext()->log(Gek::Context::Info, "X11 Window Created");

                std::atomic_bool created = false;
                std::thread create([this, &created](void) -> void
                {
                    onCreated();
                    created.store(true);
                });

                while (!stop.load())
                {
                    ::Window child = 0;
                    
                    XEvent event;
                    XNextEvent(display, &event);
                    switch(event.type)
                    {
                    case Expose:
                        break;

                    case ConfigureNotify:
                        if (true)
                        {
                            if (event.xconfigure.width != windowRectangle.size.x || event.xconfigure.height != windowRectangle.size.y)
                            {
                                windowRectangle.size.x = event.xconfigure.width;
                                windowRectangle.size.y = event.xconfigure.height;
                                getContext()->log(Gek::Context::Info, "Size Changed: {}, {}", windowRectangle.size.x, windowRectangle.size.y);
                                onSizeChanged(false);
                            }
                            else if (event.xconfigure.x != windowRectangle.position.x || event.xconfigure.y != windowRectangle.position.y)
                            {
                                windowRectangle.position.x = event.xconfigure.x;
                                windowRectangle.position.y = event.xconfigure.y;
                                getContext()->log(Gek::Context::Info, "Window Moved: {}, {}", windowRectangle.position.x, windowRectangle.position.y);
                            }
                        }

                        break;

                    case KeyPress:
                        getContext()->log(Gek::Context::Info, "You pressed the {} - {} key!\n", event.xkey.keycode, event.xkey.state);
                        onKeyPressed(ConvertKey(event.xkey.keycode), true);
                        break;

                    case KeyRelease:
                        getContext()->log(Gek::Context::Info, "You released the {} - {} key!\n", event.xkey.keycode, event.xkey.state);
                        onKeyPressed(ConvertKey(event.xkey.keycode), false);
                        break;

                    case ButtonPress:
                        XTranslateCoordinates(display, window, rootWindow, event.xbutton.x, event.xbutton.y, &event.xbutton.x, &event.xbutton.y, &child);
                        getContext()->log(Gek::Context::Info, "You pressed a button at {},{} - {} - {}\n", event.xbutton.x, event.xbutton.y, event.xbutton.button, event.xbutton.state);
                        switch(event.xbutton.button)
                        {
                        case Button1:
                            onMouseClicked(Window::Button::Left, true);
                            break;

                        case Button2:
                            onMouseClicked(Window::Button::Middle, true);
                            break;

                        case Button3:
                            onMouseClicked(Window::Button::Right, true);
                            break;

                        case Button4:
                            onMouseWheel(1.0f);
                            break;

                        case Button5:
                            onMouseWheel(-1.0f);
                            break;
                        };

                        break;

                    case ButtonRelease:
                        XTranslateCoordinates(display, window, rootWindow, event.xbutton.x, event.xbutton.y, &event.xbutton.x, &event.xbutton.y, &child);
                        getContext()->log(Gek::Context::Info, "You released a button at {},{} - {} - {}\n", event.xbutton.x, event.xbutton.y, event.xbutton.button, event.xbutton.state);
                        switch(event.xbutton.button)
                        {
                        case Button1:
                            onMouseClicked(Window::Button::Left, false);
                            break;

                        case Button2:
                            onMouseClicked(Window::Button::Middle, false);
                            break;

                        case Button3:
                            onMouseClicked(Window::Button::Right, false);
                            break;
                        };

                        break;
                    };

                    if (created.load())
                    {
                        onIdle();
                    }
                };

                create.join();
            }

            void close(void)
            {
                stop.store(true);
            }

            void redraw(void)
            {
                XEvent event{};
                event.type = Expose;
                event.xexpose.display = display;
                XSendEvent(display, window, False, ExposureMask, &event);
            }

            void *getWindowData(uint32_t data) const
            {
                switch(data)
                {
                case 0:
                    return reinterpret_cast<void *>(display);

                case 1:
                    return const_cast<void *>(reinterpret_cast<const void *>(&window));
                };

                return nullptr;
            }

            Math::Int4 getClientRectangle(bool moveToScreen = false) const
            {
                XWindowAttributes attributes;
                XGetWindowAttributes(display, window, &attributes);

                Math::Int4 rectangle;
                rectangle.position.x = attributes.x;
                rectangle.position.y = attributes.y;
                rectangle.size.x = attributes.width;
                rectangle.size.y = attributes.height;
                return rectangle;
            }

            Math::Int4 getScreenRectangle(void) const
            {
                ::Window child = 0;
                XWindowAttributes attributes;
                XGetWindowAttributes(display, window, &attributes);
                XTranslateCoordinates(display, window, rootWindow, 0, 0, &attributes.x, &attributes.y, &child);

                Math::Int4 rectangle;
                rectangle.position.x = attributes.x - attributes.border_width / 2;
                rectangle.position.y = attributes.y - attributes.border_width / 2;
                rectangle.size.x = attributes.width + attributes.border_width;
                rectangle.size.y = attributes.height + attributes.border_width;
                return rectangle;
            }

            Math::Int2 getCursorPosition(void) const
            {
                Math::Int2 position;
                return position;
            }

            void setCursorPosition(Math::Int2 const &position)
            {
            }

            void setCursorVisibility(bool isVisible)
            {
            }

            void setVisibility(bool isVisible)
            {
            }

            void move(Math::Int2 const &position)
            {
                int32_t x = position.x;
                int32_t y = position.y;
                if (x < 0 || y < 0)
                {
                    XWindowAttributes attributes;
                    XGetWindowAttributes(display, window, &attributes);
                    x = (x < 0 ? ((XWidthOfScreen(attributes.screen) - (windowRectangle.size.x + attributes.border_width)) / 2) : x);
                    y = (y < 0 ? ((XHeightOfScreen(attributes.screen) - (windowRectangle.size.y + attributes.border_width)) / 2) : y);
                }

                getContext()->log(Gek::Context::Info, "Moving: {}, {}", x, y);
                XMoveWindow(display, window, x, y);
                redraw();
            }

            void resize(Math::Int2 const &size)
            {
                getContext()->log(Gek::Context::Info, "Resizing: {}, {}", size.width, size.height);
                XResizeWindow(display, window, size.width, size.height);
                redraw();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace X11
}; // namespace Gek
