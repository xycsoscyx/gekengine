#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\PopulationInterface.h"

namespace Gek
{
    namespace Engine
    {
        namespace Population
        {
            DECLARE_REGISTERED_CLASS(System);
        }; // namespace Population
    }; // namespace Engine
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(Gek::Engine::Population::Class, Gek::Engine::Population::System)
END_CONTEXT_SOURCE