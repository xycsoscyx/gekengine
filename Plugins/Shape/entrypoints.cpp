#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(Shape);
    GEK_DECLARE_CONTEXT_USER(ShapeProcessor);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Shape, Shape);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::ShapeProcessor, ShapeProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
