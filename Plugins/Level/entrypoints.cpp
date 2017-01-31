#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(Level);
    GEK_DECLARE_CONTEXT_USER(LevelProcessor);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Level, Level);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::LevelProcessor, LevelProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
