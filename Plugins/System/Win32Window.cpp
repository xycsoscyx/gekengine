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
            }

            ~Window(void)
            {
            }
        };

        GEK_REGISTER_CONTEXT_USER(Window);
    }; // namespace Win32
}; // namespace Gek
