#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    TransformComponent::TransformComponent(void)
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
        position = loadParameter(componentData, L"position", Math::Float3::Zero);
        rotation = loadParameter(componentData, L"rotation", Math::Quaternion::Identity);
        scale = loadParameter(componentData, L"scale", Math::Float3::One);
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