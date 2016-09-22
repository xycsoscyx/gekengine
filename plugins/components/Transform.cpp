#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        void Transform::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"position", position);
            saveParameter(componentData, L"rotation", rotation);
            saveParameter(componentData, L"scale", scale);
        }

        void Transform::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            position = loadParameter(componentData, L"position", Math::Float3::Zero);
            rotation = loadParameter(componentData, L"rotation", Math::Quaternion::Identity);
            scale = loadParameter(componentData, L"scale", Math::Float3::One);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Transform)
        , public Plugin::ComponentMixin<Components::Transform>
    {
    public:
        Transform(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"transform";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Transform);
}; // namespace Gek