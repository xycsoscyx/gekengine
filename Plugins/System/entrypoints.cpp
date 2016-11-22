#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    namespace DirectInput8
    {
        GEK_DECLARE_CONTEXT_USER(System);
    };

	namespace DirectSound8
	{
		GEK_DECLARE_CONTEXT_USER(Device);
	};

    namespace Direct3D11
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    namespace Direct3D12
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    namespace OpenGL
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    namespace Vulkan
    {
        GEK_DECLARE_CONTEXT_USER(Device);
    };

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(Default::System::Input, DirectInput8::System);
		GEK_CONTEXT_ADD_CLASS(Default::Device::Audio, DirectSound8::Device);
        GEK_CONTEXT_ADD_CLASS(Default::Device::Video, Direct3D11::Device);

        GEK_CONTEXT_ADD_CLASS(System::Input::DI8, DirectInput8::System);
		GEK_CONTEXT_ADD_CLASS(System::Audio::DS8, DirectSound8::Device);
        GEK_CONTEXT_ADD_CLASS(Device::Video::D3D11, Direct3D11::Device);
        GEK_CONTEXT_ADD_CLASS(Device::Video::D3D12, Direct3D12::Device);
        GEK_CONTEXT_ADD_CLASS(Device::Video::OGL, OpenGL::Device);
        GEK_CONTEXT_ADD_CLASS(Device::Video::VLKN, Vulkan::Device);
    GEK_CONTEXT_END();
}; // namespace Gek