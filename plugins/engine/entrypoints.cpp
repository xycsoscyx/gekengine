#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\CoreInterface.h"

namespace Gek
{
    namespace Engine
    {
        namespace Population
        {
            DECLARE_REGISTERED_CLASS(System);
        }; // namespace Population

        namespace Render
        {
            DECLARE_REGISTERED_CLASS(System);
        }; // namespace Render

        namespace Core
        {
            DECLARE_REGISTERED_CLASS(System);
        }; // namespace Core
    }; // namespace Engine
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(Gek::Engine::Population::Class, Gek::Engine::Population::System)
    ADD_CONTEXT_CLASS(Gek::Engine::Render::Class, Gek::Engine::Render::System)
    ADD_CONTEXT_CLASS(Gek::Engine::Core::Class, Gek::Engine::Core::System)
END_CONTEXT_SOURCE