#include "GEK\Context\ContextUser.h"

namespace Gek
{
    namespace Sprites
    {
        GEK_DECLARE_CONTEXT_USER(Explosion);
        GEK_DECLARE_CONTEXT_USER(EmitterProcessor);
    }; // namespace Sprites

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Sprites::Explosion, Sprites::Explosion);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::Sprites::EmitterProcessor, Sprites::EmitterProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
