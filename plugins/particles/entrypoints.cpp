#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(Particles);
    GEK_DECLARE_CONTEXT_USER(ParticlesProcessor);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Particles, Particles);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::ParticlesProcessor, ParticlesProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
