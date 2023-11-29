#include "GEK/Utility/ContextUser.hpp"

#ifdef _WIN32
#include <Windows.h>
static HINSTANCE GlobalDLLInstance = nullptr;
BOOL WINAPI DllMain(HINSTANCE dllInstance, DWORD callReason, void *reserved)
{
    switch (callReason)
    {
    case DLL_PROCESS_ATTACH:
        GlobalDLLInstance = dllInstance;
        return true;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    };

    return false;
}
#endif

namespace Gek
{
#ifdef _WIN32
    HINSTANCE GetDLLInstance(void)
    {
        return GlobalDLLInstance;
    }
#endif

    namespace Window::Implementation
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    namespace Render::Implementation
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(Default::Render::Window, Window::Implementation::Device);
        GEK_CONTEXT_ADD_CLASS(Default::Render::Device, Render::Implementation::Device);
    GEK_CONTEXT_END();
}; // namespace Gek