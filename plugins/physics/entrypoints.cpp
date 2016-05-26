#include "GEK\Context\ContextUser.h"

GEK_CONTEXT_BEGIN(Components)
    GEK_CONTEXT_ADD_CLASS(Gek::MassRegistration, Gek::MassImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::RigidBodyRegistration, Gek::RigidBodyImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::StaticBodyRegistration, Gek::StaticBodyImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::PlayerBodyRegistration, Gek::PlayerBodyImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::NewtonProcessorRegistration, Gek::NewtonProcessorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ProcessorType)
GEK_CONTEXT_END()
