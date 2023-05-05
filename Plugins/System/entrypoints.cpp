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

    namespace Win32
    {
        GEK_DECLARE_CONTEXT_USER(Window);
    };

    namespace DirectSound8
	{
		GEK_DECLARE_CONTEXT_USER(Device);
	};

    namespace Direct3D11
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };
#else
    namespace X11
    {
        GEK_DECLARE_CONTEXT_USER(Window);
    };

    namespace Vulkan
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };
#endif

    GEK_CONTEXT_BEGIN(System);
#ifdef _WIN32
        GEK_CONTEXT_ADD_CLASS(Default::System::Window, Win32::Window);
		GEK_CONTEXT_ADD_CLASS(Default::Device::Audio, DirectSound8::Device);
        GEK_CONTEXT_ADD_CLASS(Default::Device::Video, Direct3D11::Device);
#else
        GEK_CONTEXT_ADD_CLASS(Default::System::Window, X11::Window);
        GEK_CONTEXT_ADD_CLASS(Default::Device::Video, Vulkan::Device);
#endif
    GEK_CONTEXT_END();
}; // namespace Gek