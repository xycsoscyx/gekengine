#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(Mass);
    GEK_DECLARE_CONTEXT_USER(RigidBody);
    GEK_DECLARE_CONTEXT_USER(StaticBody);
    GEK_DECLARE_CONTEXT_USER(PlayerBody);
    GEK_DECLARE_CONTEXT_USER(NewtonProcessor);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Mass, Mass);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::RigidBody, RigidBody);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::StaticBody, StaticBody);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::PlayerBody, PlayerBody);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::NewtonProcessor, NewtonProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
