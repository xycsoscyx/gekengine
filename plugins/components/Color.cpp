#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ColorComponent::ColorComponent(void)
    {
    }

    HRESULT ColorComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        return S_OK;
    }

    HRESULT ColorComponent::load(const Population::ComponentDefinition &componentData)
    {
        return (loadParameter(componentData, nullptr, value) ? S_OK : E_INVALIDARG);
    }

    class ColorImplementation
        : public ContextRegistration<ColorImplementation>
        , public ComponentMixin<ColorComponent>
    {
    public:
        ColorImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"color";
        }
    };

    GEK_REGISTER_CONTEXT_USER(ColorImplementation);
}; // namespace Gek