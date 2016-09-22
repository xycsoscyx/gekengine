#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        void PointLight::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"range", range);
            saveParameter(componentData, L"radius", radius);
        }

        void PointLight::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            range = loadParameter(componentData, L"range", 0.0f);
            radius = loadParameter(componentData, L"radius", 0.0f);
        }

        void SpotLight::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"range", range);
            saveParameter(componentData, L"radius", radius);
            saveParameter(componentData, L"inner_angle", Math::convertRadiansToDegrees(std::acos(innerAngle) * 2.0f));
            saveParameter(componentData, L"outer_angle", Math::convertRadiansToDegrees(std::acos(outerAngle) * 2.0f));
            saveParameter(componentData, L"falloff", falloff);
        }

        void SpotLight::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            range = loadParameter(componentData, L"range", 0.0f);
            radius = loadParameter(componentData, L"radius", 0.0f);
            innerAngle = std::cos(Math::convertDegreesToRadians(loadParameter(componentData, L"inner_angle", 0.0f) * 0.5f));
            outerAngle = std::cos(Math::convertDegreesToRadians(loadParameter(componentData, L"outer_angle", 0.0f) * 0.5f));
            falloff = loadParameter(componentData, L"falloff", 0.0f);
        }

        void DirectionalLight::save(Plugin::Population::ComponentDefinition &componentData) const
        {
        }

        void DirectionalLight::load(const Plugin::Population::ComponentDefinition &componentData)
        {
        }
    }; // namespace Components

    GEK_CONTEXT_USER(PointLight)
        , public Plugin::ComponentMixin<Components::PointLight>
    {
    public:
        PointLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"point_light";
        }
    };

    GEK_CONTEXT_USER(SpotLight)
        , public Plugin::ComponentMixin<Components::SpotLight>
    {
    public:
        SpotLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spot_light";
        }
    };

    GEK_CONTEXT_USER(DirectionalLight)
        , public Plugin::ComponentMixin<Components::DirectionalLight>
    {
    public:
        DirectionalLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"directional_light";
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek