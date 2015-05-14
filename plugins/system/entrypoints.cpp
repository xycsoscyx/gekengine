#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"

#include "System\AudioInterface.h"

namespace Gek
{
    namespace Audio
    {
        DECLARE_REGISTERED_CLASS(System)
    }; // namespace Audio
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(System)
    ADD_CONTEXT_CLASS(Gek::Audio::Class, Gek::Audio::System)
END_CONTEXT_SOURCE