#include "GEK/Utility/ContextUser.hpp"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(Model);
    GEK_DECLARE_CONTEXT_USER(ModelProcessor);

    GEK_CONTEXT_BEGIN(Engine);
        GEK_CONTEXT_ADD_CLASS(Components::Model, Model);
            GEK_CONTEXT_ADD_TYPE(ComponentType)
        GEK_CONTEXT_ADD_CLASS(Processors::ModelProcessor, ModelProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END()
}; // namespace Gek
