#include "GEK\Engine\Model.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ModelComponent::ModelComponent(void)
    {
    }

    void ModelComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        saveParameter(componentData, L"skin", skin);
    }

    void ModelComponent::load(const Population::ComponentDefinition &componentData)
    {
        value = loadParameter<String>(componentData, nullptr);
        skin = loadParameter<String>(componentData, L"skin");
    }

    class ModelImplementation
        : public ContextRegistration<ModelImplementation>
        , public ComponentMixin<ModelComponent>
    {
    public:
        ModelImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"model";
        }
    };

    GEK_REGISTER_CONTEXT_USER(ModelImplementation)
}; // namespace Gek