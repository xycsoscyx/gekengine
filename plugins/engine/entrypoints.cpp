#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Population
    {
        DECLARE_REGISTERED_CLASS(System);
    }; // namespace Population
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(Gek::PopulationSystem, Gek::Population::System)
END_CONTEXT_SOURCE