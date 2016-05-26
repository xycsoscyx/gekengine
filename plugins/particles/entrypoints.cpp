#include "GEK\Context\ContextUser.h"

GEK_CONTEXT_BEGIN(Components)
    GEK_CONTEXT_ADD_CLASS(Gek::ParticlesRegistration, Gek::ParticlesImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::ParticlesProcessorRegistration, Gek::ParticlesProcessorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ProcessorType)
GEK_CONTEXT_END()
