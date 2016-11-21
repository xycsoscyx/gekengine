#include "GEK\Utility\ContextUser.hpp"

namespace Gek
{
    namespace Implementation
    {
        GEK_DECLARE_CONTEXT_USER(Population);
        GEK_DECLARE_CONTEXT_USER(Renderer);
        GEK_DECLARE_CONTEXT_USER(Resources);
        GEK_DECLARE_CONTEXT_USER(Visual);
        GEK_DECLARE_CONTEXT_USER(Material);
        GEK_DECLARE_CONTEXT_USER(Shader);
        GEK_DECLARE_CONTEXT_USER(Filter);
        GEK_DECLARE_CONTEXT_USER(Editor);
        GEK_DECLARE_CONTEXT_USER(Core);
    };

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Engine::Population, Implementation::Population);
        GEK_CONTEXT_ADD_CLASS(Engine::Renderer, Implementation::Renderer);
        GEK_CONTEXT_ADD_CLASS(Engine::Resources, Implementation::Resources);
        GEK_CONTEXT_ADD_CLASS(Engine::Visual, Implementation::Visual);
        GEK_CONTEXT_ADD_CLASS(Engine::Material, Implementation::Material);
        GEK_CONTEXT_ADD_CLASS(Engine::Shader, Implementation::Shader);
        GEK_CONTEXT_ADD_CLASS(Engine::Filter, Implementation::Filter);
        GEK_CONTEXT_ADD_CLASS(Engine::Editor, Implementation::Editor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
        GEK_CONTEXT_ADD_CLASS(Engine::Core, Implementation::Core);
    GEK_CONTEXT_END()
}; // namespace Gek
