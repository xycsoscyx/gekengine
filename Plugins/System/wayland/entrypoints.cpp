#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    namespace Window::Implementation
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    GEK_CONTEXT_BEGIN(System);
    GEK_CONTEXT_ADD_CLASS(Default::System::Window, Window::Implementation::Device);
    GEK_CONTEXT_END();
}; // namespace Gek
