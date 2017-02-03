#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    namespace Win32
    {
        GEK_DECLARE_CONTEXT_USER(Window);
    };

    namespace Direct3D11
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(Default::System::Window, Win32::Window);
        GEK_CONTEXT_ADD_CLASS(Default::Render::Device, Direct3D11::Device);
    GEK_CONTEXT_END();
}; // namespace Gek