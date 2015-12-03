#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Engine.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(PopulationImplementation);
    DECLARE_REGISTERED_CLASS(RenderImplementation);
    DECLARE_REGISTERED_CLASS(PluginImplementation);
    DECLARE_REGISTERED_CLASS(MaterialImplementation);
    DECLARE_REGISTERED_CLASS(ShaderImplementation);
    DECLARE_REGISTERED_CLASS(EngineImplementation);
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Engine)
    ADD_CONTEXT_CLASS(Gek::PopulationRegistration, Gek::PopulationImplementation)
    ADD_CONTEXT_CLASS(Gek::RenderRegistration, Gek::RenderImplementation)
    ADD_CONTEXT_CLASS(Gek::PluginRegistration, Gek::PluginImplementation)
    ADD_CONTEXT_CLASS(Gek::MaterialRegistration, Gek::MaterialImplementation)
    ADD_CONTEXT_CLASS(Gek::ShaderRegistration, Gek::ShaderImplementation)
    ADD_CONTEXT_CLASS(Gek::EngineRegistration, Gek::EngineImplementation)
END_CONTEXT_SOURCE