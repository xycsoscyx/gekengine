#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    TransformComponent::TransformComponent(void)
        : position(0.0f, 0.0f, 0.0f)
        , rotation(0.0f, 0.0f, 0.0f, 1.0f)
        , scale(1.0f, 1.0f, 1.0f)
    {
    }

    HRESULT TransformComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"position", position);
        saveParameter(componentData, L"rotation", rotation);
        saveParameter(componentData, L"scale", scale);
        return S_OK;
    }

    HRESULT TransformComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"position", position);
        loadParameter(componentData, L"rotation", rotation);
        loadParameter(componentData, L"scale", scale);
        return S_OK;
    }

    class TransformImplementation
        : public ContextUserMixin
        , public ComponentMixin<TransformComponent>
    {
    public:
        TransformImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(TransformImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component
        const char *getName(void) const
        {
            return L"transform";
        }
    };

    REGISTER_CLASS(TransformImplementation)
}; // namespace Gek