#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\System\InputInterface.h"
#include "GEK\System\AudioInterface.h"
#include "GEK\System\VideoInterface.h"

namespace Gek
{
    namespace Input
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Input

    namespace Audio
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Audio

    namespace Video
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Video
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(Gek::InputSystem, Gek::Input::System)
    ADD_CONTEXT_CLASS(Gek::AudioSystem, Gek::Audio::System)
    ADD_CONTEXT_CLASS(Gek::VideoSystem, Gek::Video::System)
END_CONTEXT_SOURCE