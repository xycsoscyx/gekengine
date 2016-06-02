#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

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
        loadParameter(componentData, L"range", range);
        loadParameter(componentData, L"radius", radius);
    }

    SpotLightComponent::SpotLightComponent(void)
    {
    }

    void SpotLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"range", range);
        saveParameter(componentData, L"radius", radius);
        saveParameter(componentData, L"inner_angle", innerAngle);
        saveParameter(componentData, L"outer_angle", outerAngle);
    }

    void SpotLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"range", range);
        loadParameter(componentData, L"radius", radius);
        loadParameter(componentData, L"inner_angle", innerAngle);
        loadParameter(componentData, L"outer_angle", outerAngle);
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