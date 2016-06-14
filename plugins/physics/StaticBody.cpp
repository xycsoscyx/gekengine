#include "GEK\Newton\StaticBody.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    StaticBodyComponent::StaticBodyComponent(void)
    {
    }

    void StaticBodyComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, shape);
    }

    void StaticBodyComponent::load(const Population::ComponentDefinition &componentData)
    {
        shape = loadParameter<String>(componentData, nullptr);
    }

    class StaticBodyImplementation
        : public ContextRegistration<StaticBodyImplementation>
        , public ComponentMixin<StaticBodyComponent>
    {
    public:
        StaticBodyImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"static_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(StaticBodyImplementation)
}; // namespace Gek