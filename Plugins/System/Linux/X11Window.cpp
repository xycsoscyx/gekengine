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
            switch (key)
            {
            case XK_A:      return Window::Key::A;
            case XK_B:      return Window::Key::B;
            case XK_C:      return Window::Key::C;
            case XK_D:      return Window::Key::D;
            case XK_E:      return Window::Key::E;
            case XK_F:      return Window::Key::F;
            case XK_G:      return Window::Key::G;
            case XK_H:      return Window::Key::H;
            case XK_I:      return Window::Key::I;
            case XK_J:      return Window::Key::J;
            case XK_K:      return Window::Key::K;
            case XK_L:      return Window::Key::L;
            case XK_M:      return Window::Key::M;
            case XK_N:      return Window::Key::N;
            case XK_O:      return Window::Key::O;
            case XK_P:      return Window::Key::P;
            case XK_Q:      return Window::Key::Q;
            case XK_R:      return Window::Key::R;
            case XK_S:      return Window::Key::S;
            case XK_T:      return Window::Key::T;
            case XK_U:      return Window::Key::U;
            case XK_V:      return Window::Key::V;
            case XK_W:      return Window::Key::W;
            case XK_X:      return Window::Key::X;
            case XK_Y:      return Window::Key::Y;
            case XK_Z:      return Window::Key::Z;
            case XK_1:      return Window::Key::Key1;
            case XK_2:      return Window::Key::Key2;
            case XK_3:      return Window::Key::Key3;
            case XK_4:      return Window::Key::Key4;
            case XK_5:      return Window::Key::Key5;
            case XK_6:      return Window::Key::Key6;
            case XK_7:      return Window::Key::Key7;
            case XK_8:      return Window::Key::Key8;
            case XK_9:      return Window::Key::Key9;
            case XK_0:      return Window::Key::Key0;
            case XK_Return:      return Window::Key::Return;
            case XK_Escape:      return Window::Key::Escape;
            case XK_BackSpace:      return Window::Key::Backspace;
            case XK_Tab:      return Window::Key::Tab;
            case XK_space:      return Window::Key::Space;
            case XK_minus:      return Window::Key::Minus;
            case XK_equal:      return Window::Key::Equals;
            case XK_bracketleft:      return Window::Key::LeftBracket;
            case XK_bracketright:      return Window::Key::RightBracket;
            case XK_backslash:      return Window::Key::Backslash;
            case XK_semicolon:      return Window::Key::Semicolon;
            case XK_quotedbl:      return Window::Key::Quote;
            case XK_grave:      return Window::Key::Grave;
            case XK_comma:      return Window::Key::Comma;
            case XK_period:      return Window::Key::Period;
            case XK_slash:      return Window::Key::Slash;
            case XK_Caps_Lock:      return Window::Key::CapsLock;
            case XK_F1:      return Window::Key::F1;
            case XK_F2:      return Window::Key::F2;
            case XK_F3:      return Window::Key::F3;
            case XK_F4:      return Window::Key::F4;
            case XK_F5:      return Window::Key::F5;
            case XK_F6:      return Window::Key::F6;
            case XK_F7:      return Window::Key::F7;
            case XK_F8:      return Window::Key::F8;
            case XK_F9:      return Window::Key::F9;
            case XK_F10:      return Window::Key::F10;
            case XK_F11:      return Window::Key::F11;
            case XK_F12:      return Window::Key::F12;
            case XK_Print:      return Window::Key::PrintScreen;
            case XK_Scroll_Lock:      return Window::Key::ScrollLock;
            case XK_Pause:      return Window::Key::Pause;
            case XK_Insert:      return Window::Key::Insert;
            case XK_Delete:      return Window::Key::Delete;
            case XK_Home:      return Window::Key::Home;
            case XK_End:      return Window::Key::End;
            case XK_Page_Up:      return Window::Key::PageUp;
            case XK_Page_Down:      return Window::Key::PageDown;
            case XK_Right:      return Window::Key::Right;
            case XK_Left:      return Window::Key::Left;
            case XK_Down:      return Window::Key::Down;
            case XK_Up:      return Window::Key::Up;
            case XK_Num_Lock:      return Window::Key::KeyPadNumLock;
            case XK_KP_Divide:      return Window::Key::KeyPadDivide;
            case XK_KP_Multiply:      return Window::Key::KeyPadMultiply;
            case XK_KP_Subtract:      return Window::Key::KeyPadSubtract;
            case XK_KP_Add:      return Window::Key::KeyPadAdd;
            case XK_KP_1:      return Window::Key::KeyPad1;
            case XK_KP_2:      return Window::Key::KeyPad2;
            case XK_KP_3:      return Window::Key::KeyPad3;
            case XK_KP_4:      return Window::Key::KeyPad4;
            case XK_KP_5:      return Window::Key::KeyPad5;
            case XK_KP_6:      return Window::Key::KeyPad6;
            case XK_KP_7:      return Window::Key::KeyPad7;
            case XK_KP_8:      return Window::Key::KeyPad8;
            case XK_KP_9:      return Window::Key::KeyPad9;
            case XK_KP_0:      return Window::Key::KeyPad0;
            case XK_KP_Decimal:      return Window::Key::KeyPadPoint;
            case XK_Control_L:      return Window::Key::LeftControl;
            case XK_Shift_L:      return Window::Key::LeftShift;
            case XK_Alt_L:      return Window::Key::LeftAlt;
            case XK_Control_R:      return Window::Key::RightControl;
            case XK_Shift_R:      return Window::Key::RightShift;
            case XK_Alt_R:      return Window::Key::RightAlt;
            };

            return Window::Key::Unknown;
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
                redraw();
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
                Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", 1);
                XSetWMProtocols(display, window, &wmDelete, 1);

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

                getContext()->log(Gek::Context::Info, "Window Created");

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

                    case ClientMessage:
                        if (!strcmp(XGetAtomName(display, event.xclient.message_type), "WM_PROTOCOLS"))
                        {
                            onCloseRequested();
                        }

                        break;

                    case ConfigureNotify:
                        if (true)
                        {
                            if (event.xconfigure.width != windowRectangle.size.x || event.xconfigure.height != windowRectangle.size.y)
                            {
                                windowRectangle.size.x = event.xconfigure.width;
                                windowRectangle.size.y = event.xconfigure.height;
                                //getContext()->log(Gek::Context::Info, "Size Changed: {}, {}", windowRectangle.size.x, windowRectangle.size.y);
                                onSizeChanged(false);
                            }
                            else if (event.xconfigure.x != windowRectangle.position.x || event.xconfigure.y != windowRectangle.position.y)
                            {
                                windowRectangle.position.x = event.xconfigure.x;
                                windowRectangle.position.y = event.xconfigure.y;
                                //getContext()->log(Gek::Context::Info, "Window Moved: {}, {}", windowRectangle.position.x, windowRectangle.position.y);
                            }
                        }

                        break;

                    case KeyPress:
                        //getContext()->log(Gek::Context::Info, "You pressed the {} - {} key!\n", event.xkey.keycode, event.xkey.state);
                        onKeyPressed(ConvertKey(event.xkey.keycode, event.xkey.state), true);
                        break;

                    case KeyRelease:
                        //getContext()->log(Gek::Context::Info, "You released the {} - {} key!\n", event.xkey.keycode, event.xkey.state);
                        onKeyPressed(ConvertKey(event.xkey.keycode, event.xkey.state), false);
                        break;

                    case ButtonPress:
                        XTranslateCoordinates(display, window, rootWindow, event.xbutton.x, event.xbutton.y, &event.xbutton.x, &event.xbutton.y, &child);
                        //getContext()->log(Gek::Context::Info, "You pressed a button at {},{} - {} - {}\n", event.xbutton.x, event.xbutton.y, event.xbutton.button, event.xbutton.state);
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
                        //getContext()->log(Gek::Context::Info, "You released a button at {},{} - {} - {}\n", event.xbutton.x, event.xbutton.y, event.xbutton.button, event.xbutton.state);
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

                XMoveWindow(display, window, x, y);
                redraw();
            }

            void resize(Math::Int2 const &size)
            {
                XResizeWindow(display, window, size.width, size.height);
                redraw();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace X11
}; // namespace Gek
