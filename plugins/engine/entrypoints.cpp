#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(PopulationImplementation);
    GEK_DECLARE_CONTEXT_USER(RenderImplementation);
    GEK_DECLARE_CONTEXT_USER(ResourcesImplementation);
    GEK_DECLARE_CONTEXT_USER(PluginImplementation);
    GEK_DECLARE_CONTEXT_USER(MaterialImplementation);
    GEK_DECLARE_CONTEXT_USER(ShaderImplementation);
    GEK_DECLARE_CONTEXT_USER(EngineImplementation);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(PopulationSystem, PopulationImplementation);
        GEK_CONTEXT_ADD_CLASS(RenderSystem, RenderImplementation);
        GEK_CONTEXT_ADD_CLASS(ResourcesSystem, ResourcesImplementation);
        GEK_CONTEXT_ADD_CLASS(PluginSystem, PluginImplementation);
        GEK_CONTEXT_ADD_CLASS(MaterialSystem, MaterialImplementation);
        GEK_CONTEXT_ADD_CLASS(ShaderSystem, ShaderImplementation);
        GEK_CONTEXT_ADD_CLASS(EngineSystem, EngineImplementation);
    GEK_CONTEXT_END()
}; // namespace Gek
