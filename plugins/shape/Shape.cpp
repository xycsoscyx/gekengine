#include "GEK\Engine\Shape.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ShapeComponent::ShapeComponent(void)
    {
    }

    void ShapeComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        saveParameter(componentData, L"parameters", parameters);
        saveParameter(componentData, L"skin", skin);
    }

    void ShapeComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, value);
        loadParameter(componentData, L"parameters", parameters);
        loadParameter(componentData, L"skin", skin);
    }

    class ShapeImplementation
        : public ContextRegistration<ShapeImplementation>
        , public ComponentMixin<ShapeComponent>
    {
    public:
        ShapeImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"shape";
        }
    };

    GEK_REGISTER_CONTEXT_USER(ShapeImplementation)
}; // namespace Gek