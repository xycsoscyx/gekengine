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
        : public ContextRegistration<MassImplementation>
        , public ComponentMixin<MassComponent>
    {
    public:
        MassImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"mass";
        }
    };

    GEK_REGISTER_CONTEXT_USER(MassImplementation)
}; // namespace Gek