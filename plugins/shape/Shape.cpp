#include "GEK\Engine\Shape.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ShapeComponent::ShapeComponent(void)
    {
    }

    HRESULT ShapeComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, value);
        saveParameter(componentData, L"parameters", parameters);
        saveParameter(componentData, L"skin", skin);
        return S_OK;
    }

    HRESULT ShapeComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, value);
        loadParameter(componentData, L"parameters", parameters);
        loadParameter(componentData, L"skin", skin);
        return S_OK;
    }

    class ShapeImplementation
        : public ContextUserMixin
        , public ComponentMixin<ShapeComponent>
    {
    public:
        ShapeImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ShapeImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        const char *getName(void) const
        {
            return L"shape";
        }
    };

    REGISTER_CLASS(ShapeImplementation)
}; // namespace Gek