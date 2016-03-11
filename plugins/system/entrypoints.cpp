#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
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
    ADD_CONTEXT_CLASS(Gek::InputSystemRegistration, Gek::InputSystemImplementation)
    ADD_CONTEXT_CLASS(Gek::AudioSystemRegistration, Gek::AudioSystemImplementation)
    ADD_CONTEXT_CLASS(Gek::VideoSystemRegistration, Gek::VideoSystemImplementation)
END_CONTEXT_SOURCE