#include "GEK/System/Window.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

namespace Gek
{
    namespace X11
    {
        GEK_CONTEXT_USER(Window, Gek::Window::Description)
            , public Gek::Window
        {
        private:
            int screen = 0;
            Display *display = nullptr;
            ::Window window = 0;
            GC graphicContext = nullptr;

        public:
            Window(Context *context, Window::Description description)
                : ContextRegistration(context)
            {
                display = XOpenDisplay(nullptr);
                screen = DefaultScreen(display);
                auto black = BlackPixel(display, screen);
                auto white = WhitePixel(display, screen);

                window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,	200, 300, 5, white, black);

                XSetStandardProperties(display, window, description.windowName.data(), "GEK", None, nullptr, 0, nullptr);

                XSelectInput(display, window, ExposureMask|ButtonPressMask|KeyPressMask);

                graphicContext = XCreateGC(display, window, 0, 0);        

                XSetBackground(display, graphicContext, white);
                XSetForeground(display, graphicContext, black);

                XClearWindow(display, window);
                XMapRaised(display, window);

                XSelectInput(display, window, ExposureMask | ButtonPressMask | KeyPressMask);
                XFlush(display);

                getContext()->log(Gek::Context::Info, "X11 Window Created");
            }

            ~Window(void)
            {
                XFlush(display);
                XFreeGC(display, graphicContext);
                XDestroyWindow(display, window);
                XCloseDisplay(display);	
            }

            void readEvents(void)
            {
                XEvent event;
                KeySym key;
                char text[255];
                if (XCheckWindowEvent(display, window, 0, &event))
                {
                    switch(event.type)
                    {
                    case Expose:
                        break;

                    case KeyPress:
                        if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) 
                        {
                            printf("You pressed the %c key!\n", text[0]);
                        }

                        break;

                    case ButtonPress:
                        printf("You pressed a button at (%i,%i)\n", event.xbutton.x, event.xbutton.y);
                        break;
                    };
                }      
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
                ::Window rootWindow, currentWindow = window;
                do
                {
                    uint32_t childrenCount;
                    ::Window parentWindow, *childrenWindows;
                    XQueryTree(display, currentWindow, &rootWindow, &parentWindow, &childrenWindows, &childrenCount);
                    if (parentWindow != rootWindow)
                    {
                        currentWindow = parentWindow;
                    } 
                } while (currentWindow != rootWindow);

                XWindowAttributes attributes;
                XGetWindowAttributes(display, currentWindow, &attributes);

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

                    x = (x < 0 ? ((XWidthOfScreen(attributes.screen) - attributes.width) / 2) : x);
                    y = (y < 0 ? ((XHeightOfScreen(attributes.screen) - attributes.height) / 2) : y);
                }

                XMoveWindow(display, window, x, y);
                XFlush(display);
            }

            void resize(Math::Int2 const &size)
            {
                XResizeWindow(display, window, size.width, size.height);
                XFlush(display);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace X11
}; // namespace Gek
