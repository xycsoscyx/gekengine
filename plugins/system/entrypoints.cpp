#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Plugin.h"

namespace Gek
{
    GEK_DECLARE_PLUGIN(InputSystemImplementation);
    GEK_DECLARE_PLUGIN(AudioSystemImplementation);
    GEK_DECLARE_PLUGIN(VideoSystemImplementation);

    GEK_PLUGIN_BEGIN(System)
        GEK_PLUGIN_CLASS(L"InputSystem", InputSystemImplementation)
        GEK_PLUGIN_CLASS(L"AudioSystem", AudioSystemImplementation)
        GEK_PLUGIN_CLASS(L"VideoSystem", VideoSystemImplementation)
    GEK_PLUGIN_END()
}; // namespace Gek