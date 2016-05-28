#include "GEK\Engine\Model.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ModelComponent::ModelComponent(void)
    {
    }

    HRESULT ModelComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        saveParameter(componentData, L"skin", skin);
        return S_OK;
    }

    HRESULT ModelComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, value);
        loadParameter(componentData, L"skin", skin);
        return S_OK;
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