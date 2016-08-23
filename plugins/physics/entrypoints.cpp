#include "GEK\Context\ContextUser.h"

namespace Gek
{
    namespace Newton
    {
        GEK_DECLARE_CONTEXT_USER(Physical);
        GEK_DECLARE_CONTEXT_USER(Player);
        GEK_DECLARE_CONTEXT_USER(Processor);
    };

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Physical, Newton::Physical);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Components::Player, Newton::Player);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::Physics, Newton::Processor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
