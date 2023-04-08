#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    namespace Physics
    {
        GEK_DECLARE_CONTEXT_USER(Scene);
        GEK_DECLARE_CONTEXT_USER(Physical);
        GEK_DECLARE_CONTEXT_USER(Player);
        GEK_DECLARE_CONTEXT_USER(Processor);
    };

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Scene, Physics::Scene);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::Physical, Physics::Physical);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::Player, Physics::Player);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::Physics, Physics::Processor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
