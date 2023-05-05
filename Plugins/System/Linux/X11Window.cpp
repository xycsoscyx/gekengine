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
        GEK_CONTEXT_USER_BASE(Window)
            , public Gek::Window
        {
        private:
            int screen = 0;
            Display *display = nullptr;
            ::Window window = 0;
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

                window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,	300, 200, 5, white, black);

                XSetStandardProperties(display, window, description.windowName.data(), "GEK", None, nullptr, 0, nullptr);

                XSelectInput(display, window, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);

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
                    XEvent event;
                    XNextEvent(display, &event);
                    switch(event.type)
                    {
                    case Expose:
                        break;

                    case ConfigureNotify:
                        if (true)
                        {
                            if (event.xconfigure.x != windowRectangle.size.x || event.xconfigure.y != windowRectangle.size.y)
                            {
                                windowRectangle.size.x = event.xconfigure.width;
                                windowRectangle.size.y = event.xconfigure.height;
                                onSizeChanged(false);
                            }
                            else if (event.xconfigure.x != windowRectangle.position.x || event.xconfigure.y != windowRectangle.position.y)
                            {
                                windowRectangle.position.x = event.xconfigure.x;
                                windowRectangle.position.y = event.xconfigure.y;
                            }
                        }

                        break;

                    case KeyPress:
                        if (true)
                        {
                            KeySym key;
                            char text[255];
                            if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) 
                            {
                                printf("You pressed the %c key!\n", text[0]);
                            }
                        }

                        break;

                    case ButtonPress:
                        printf("You pressed a button at (%i,%i)\n", event.xbutton.x, event.xbutton.y);
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
                return Math::Int4(0.0f, 0.0f, windowRectangle.size.x, windowRectangle.size.y);

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
                return windowRectangle;

                /*::Window rootWindow, currentWindow = window;
                do
                {
                    uint32_t childrenCount;
                    ::Window parentWindow, *childrenWindows;
                    XQueryTree(display, currentWindow, &rootWindow, &parentWindow, &childrenWindows, &childrenCount);
                    if (parentWindow != rootWindow)
                    {
                        currentWindow = parentWindow;
                    } 
                } while (currentWindow != rootWindow);*/

                XWindowAttributes attributes;
                XGetWindowAttributes(display, window, &attributes);

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
