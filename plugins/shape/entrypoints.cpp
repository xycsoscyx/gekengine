#include "GEK\Context\ContextUser.h"

GEK_CONTEXT_BEGIN(Components)
    GEK_CONTEXT_ADD_CLASS(Gek::ShapeRegistration, Gek::ShapeImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::ShapeProcessorRegistration, Gek::ShapeProcessorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ProcessorType)
GEK_CONTEXT_END()
