#include "GEK\Newton\Mass.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    MassComponent::MassComponent(void)
        : value(0.0f)
    {
    }

    HRESULT MassComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        return S_OK;
    }

    HRESULT MassComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, value);
        return S_OK;
    }

    class MassImplementation
        : public ContextUserMixin
        , public ComponentMixin<MassComponent>
    {
    public:
        MassImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(MassImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(const wchar_t *) getName(void) const
        {
            return L"mass";
        }
    };

    REGISTER_CLASS(MassImplementation)
}; // namespace Gek