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
            Display *display = nullptr;
            int screen = 0;
            ::Window window;
            GC graphicContext;

        public:
            Window(Context *context, Window::Description description)
                : ContextRegistration(context)
            {

                /* use the information from the environment variable DISPLAY 
                to create the X connection:
                */	
                display = XOpenDisplay(nullptr);
                screen = DefaultScreen(display);
                auto black = BlackPixel(display, screen); /* get color black */
                auto white = WhitePixel(display, screen); /* get color white */

                /* once the display is initialized, create the window.
                This window will be have be 200 pixels across and 300 down.
                It will have the foreground white and background black
                */
                window = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,	200, 300, 5, white, black);

                /* here is where some properties of the window can be set.
                The third and fourth items indicate the name which appears
                at the top of the window and the name of the minimized window
                respectively.
                */
                XSetStandardProperties(display, window, description.windowName.data(), "GEK", None, nullptr, 0, nullptr);

                /* this routine determines which types of input are allowed in
                the input.  see the appropriate section for details...
                */
                XSelectInput(display, window, ExposureMask|ButtonPressMask|KeyPressMask);

                /* create the Graphics Context */
                graphicContext = XCreateGC(display, window, 0, 0);        

                /* here is another routine to set the foreground and background
                colors _currently_ in use in the window.
                */
                XSetBackground(display, graphicContext, white);
                XSetForeground(display, graphicContext, black);

                /* clear the window and bring it on top of the other windows */
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

            // Window
            void readEvents(void)
            {
                XEvent event;		/* the XEvent declaration !!! */
                KeySym key;		/* a dealie-bob to handle KeyPress Events */	
                char text[255];		/* a char buffer for KeyPress Events */

                /* get the next event and stuff it into our event variable.
                Note:  only events we set the mask for are detected!
                */
                if (XCheckWindowEvent(display, window, 0, &event))
                {
                    switch(event.type)
                    {
                    case Expose:
                        break;

                    case KeyPress:
                        if (XLookupString(&event.xkey, text, 255, &key, 0) == 1) 
                        {
                            /* use the XLookupString routine to convert the invent
                            KeyPress data into regular text.  Weird but necessary...
                            */
                            printf("You pressed the %c key!\n", text[0]);
                        }

                        break;

                    case ButtonPress:
                        /* tell where the mouse Button was Pressed */
                        printf("You pressed a button at (%i,%i)\n", event.xbutton.x, event.xbutton.y);
                        break;
                    };
                }      
            }

            void *getBaseWindow(void) const
            {
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
