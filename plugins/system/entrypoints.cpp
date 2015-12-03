#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\System\InputSystem.h"
#include "GEK\System\AudioSystem.h"
#include "GEK\System\VideoSystem.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(InputSystemImplementation);
    DECLARE_REGISTERED_CLASS(AudioSystemImplementation);
    DECLARE_REGISTERED_CLASS(VideoSystemImplementation);
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(Gek::InputSystemClass, Gek::InputSystemImplementation)
    ADD_CONTEXT_CLASS(Gek::AudioSystemClass, Gek::AudioSystemImplementation)
    ADD_CONTEXT_CLASS(Gek::VideoSystemClass, Gek::VideoSystemImplementation)
END_CONTEXT_SOURCE