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

    void TransformComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"position", position);
        saveParameter(componentData, L"rotation", rotation);
        saveParameter(componentData, L"scale", scale);
    }

    void TransformComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"position", position);
        loadParameter(componentData, L"rotation", rotation);
        loadParameter(componentData, L"scale", scale);
    }

    class TransformImplementation
        : public ContextRegistration<TransformImplementation>
        , public ComponentMixin<TransformComponent>
    {
    public:
        TransformImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"transform";
        }
    };

    GEK_REGISTER_CONTEXT_USER(TransformImplementation);
}; // namespace Gek