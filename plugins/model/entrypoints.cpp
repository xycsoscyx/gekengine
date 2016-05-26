#include "GEK\Context\ContextUser.h"

GEK_CONTEXT_BEGIN(Components)
    GEK_CONTEXT_ADD_CLASS(Gek::ModelRegistration, Gek::ModelImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::ModelProcessorRegistration, Gek::ModelProcessorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ProcessorType)
GEK_CONTEXT_END()
