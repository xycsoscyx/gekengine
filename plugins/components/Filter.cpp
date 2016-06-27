#include "GEK\Components\Filter.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    FilterComponent::FilterComponent(void)
    {
    }

    void FilterComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, String(list, L','));
    }

    void FilterComponent::load(const Population::ComponentDefinition &componentData)
    {
        list = loadParameter(componentData, nullptr, String()).split(L',');
    }

    class FilterImplementation
        : public ContextRegistration<FilterImplementation>
        , public ComponentMixin<FilterComponent>
    {
    public:
        FilterImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"filter";
        }
    };

    GEK_REGISTER_CONTEXT_USER(FilterImplementation);
}; // namespace Gek