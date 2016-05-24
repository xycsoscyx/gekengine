#include <initguid.h>
#include <cguid.h>

#include "GEK\Engine\Population.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Plugin.h"
#include "GEK\Engine\Material.h"
#include "GEK\Engine\Shader.h"
#include "GEK\Engine\Engine.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(PopulationImplementation);
    DECLARE_REGISTERED_CLASS(RenderImplementation);
    DECLARE_REGISTERED_CLASS(ResourcesImplementation);
    DECLARE_REGISTERED_CLASS(PluginImplementation);
    DECLARE_REGISTERED_CLASS(MaterialImplementation);
    DECLARE_REGISTERED_CLASS(ShaderImplementation);
    DECLARE_REGISTERED_CLASS(EngineImplementation);
}; // namespace Gek

GEK_PLUGIN_BEGIN(Engine)
    GEK_PLUGIN_CLASS(Gek::PopulationRegistration, Gek::PopulationImplementation)
    GEK_PLUGIN_CLASS(Gek::RenderRegistration, Gek::RenderImplementation)
    GEK_PLUGIN_CLASS(Gek::ResourcesRegistration, Gek::ResourcesImplementation)
    GEK_PLUGIN_CLASS(Gek::PluginRegistration, Gek::PluginImplementation)
    GEK_PLUGIN_CLASS(Gek::MaterialRegistration, Gek::MaterialImplementation)
    GEK_PLUGIN_CLASS(Gek::ShaderRegistration, Gek::ShaderImplementation)
    GEK_PLUGIN_CLASS(Gek::EngineRegistration, Gek::EngineImplementation)
GEK_PLUGIN_END
