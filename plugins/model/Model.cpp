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
        : public ContextUserMixin
        , public ComponentMixin<ModelComponent>
    {
    public:
        ModelImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ModelImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        const char *getName(void) const
        {
            return L"model";
        }
    };

    REGISTER_CLASS(ModelImplementation)
}; // namespace Gek