#include "GEK/System/Window.hpp"
#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    namespace X11
    {
        GEK_CONTEXT_USER(Window, Gek::Window::Description)
            , public Gek::Window
        {
        private:

        public:
            Window(Context *context, Window::Description description)
                : ContextRegistration(context)
            {
            }

            ~Window(void)
            {
            }

            // Window
            void readEvents(void)
            {
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
