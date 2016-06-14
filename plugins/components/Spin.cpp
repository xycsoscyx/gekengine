#include "GEK\Components\Spin.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    SpinComponent::SpinComponent(void)
    {
    }

    void SpinComponent::save(Population::ComponentDefinition &componentData) const
    {
    }

    void SpinComponent::load(const Population::ComponentDefinition &componentData)
    {
    }

    class SpinImplementation
        : public ContextRegistration<SpinImplementation>
        , public ComponentMixin<SpinComponent>
    {
    public:
        SpinImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"spin";
        }
    };

    GEK_REGISTER_CONTEXT_USER(SpinImplementation);
}; // namespace Gek