#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ColorComponent::ColorComponent(void)
    {
    }

    void ColorComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
    }

    void ColorComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, value);
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