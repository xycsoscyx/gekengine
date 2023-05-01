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
            }

            ~Window(void)
            {
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
                Math::Int4 rectangle;
                return rectangle;
            }

            Math::Int4 getScreenRectangle(void) const
            {
                Math::Int4 rectangle;
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
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace X11
}; // namespace Gek
