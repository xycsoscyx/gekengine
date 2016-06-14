#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    PointLightComponent::PointLightComponent(void)
    {
    }

    void PointLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"range", range);
        saveParameter(componentData, L"radius", radius);
    }

    void PointLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        range = loadParameter(componentData, L"range", 10.0f);
        radius = loadParameter(componentData, L"radius", 0.1f);
    }

    SpotLightComponent::SpotLightComponent(void)
    {
    }

    void SpotLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"range", range);
        saveParameter(componentData, L"radius", radius);
        saveParameter(componentData, L"inner_angle", Math::convertRadiansToDegrees(std::acos(innerAngle) * 2.0f));
        saveParameter(componentData, L"outer_angle", Math::convertRadiansToDegrees(std::acos(outerAngle) * 2.0f));
    }

    void SpotLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        range = loadParameter(componentData, L"range", 10.0f);
        radius = loadParameter(componentData, L"radius", 0.1f);
        innerAngle = std::cos(Math::convertDegreesToRadians(loadParameter(componentData, L"inner_angle", 45.0f)) * 0.5f);
        outerAngle = std::cos(Math::convertDegreesToRadians(loadParameter(componentData, L"outer_angle", 90.0f)) * 0.5f);
    }

    DirectionalLightComponent::DirectionalLightComponent(void)
    {
    }

    void DirectionalLightComponent::save(Population::ComponentDefinition &componentData) const
    {
    }

    void DirectionalLightComponent::load(const Population::ComponentDefinition &componentData)
    {
    }

    class PointLightImplementation
        : public ContextRegistration<PointLightImplementation>
        , public ComponentMixin<PointLightComponent>
    {
    public:
        PointLightImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"point_light";
        }
    };

    class SpotLightImplementation
        : public ContextRegistration<SpotLightImplementation>
        , public ComponentMixin<SpotLightComponent>
    {
    public:
        SpotLightImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"spot_light";
        }
    };

    class DirectionalLightImplementation
        : public ContextRegistration<DirectionalLightImplementation>
        , public ComponentMixin<DirectionalLightComponent>
    {
    public:
        DirectionalLightImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"directional_light";
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLightImplementation);
    GEK_REGISTER_CONTEXT_USER(SpotLightImplementation);
    GEK_REGISTER_CONTEXT_USER(DirectionalLightImplementation);
}; // namespace Gek