#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(MassImplementation);
    GEK_DECLARE_CONTEXT_USER(RigidBodyImplementation);
    GEK_DECLARE_CONTEXT_USER(StaticBodyImplementation);
    GEK_DECLARE_CONTEXT_USER(PlayerBodyImplementation);
    GEK_DECLARE_CONTEXT_USER(NewtonProcessorImplementation);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Mass, MassImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(RigidBody, RigidBodyImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(StaticBody, StaticBodyImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(PlayerBody, PlayerBodyImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(NewtonProcessor, NewtonProcessorImplementation);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
