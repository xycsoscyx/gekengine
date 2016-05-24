#include "GEK\Components\Light.h"
#include "GEK\Context\Plugin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    PointLightComponent::PointLightComponent(void)
    {
    }

    HRESULT PointLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"range", range);
        saveParameter(componentData, L"radius", radius);
        return S_OK;
    }

    HRESULT PointLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        return (loadParameter(componentData, L"range", range) &&
            loadParameter(componentData, L"radius", radius) ? S_OK : E_INVALIDARG);
    }

    SpotLightComponent::SpotLightComponent(void)
    {
    }

    HRESULT SpotLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"range", range);
        saveParameter(componentData, L"radius", radius);
        saveParameter(componentData, L"inner_angle", innerAngle);
        saveParameter(componentData, L"outer_angle", outerAngle);
        return S_OK;
    }

    HRESULT SpotLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        return (loadParameter(componentData, L"range", range) &&
            loadParameter(componentData, L"radius", radius) &&
            loadParameter(componentData, L"inner_angle", innerAngle) &&
            loadParameter(componentData, L"outer_angle", outerAngle) ? S_OK : E_INVALIDARG);
    }

    DirectionalLightComponent::DirectionalLightComponent(void)
    {
    }

    HRESULT DirectionalLightComponent::save(Population::ComponentDefinition &componentData) const
    {
        return E_NOTIMPL;
    }

    HRESULT DirectionalLightComponent::load(const Population::ComponentDefinition &componentData)
    {
        return S_OK;
    }

    class PointLightImplementation
        : public ContextUserMixin
        , public ComponentMixin<PointLightComponent>
    {
    public:
        PointLightImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PointLightImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"point_light";
        }
    };

    class SpotLightImplementation
        : public ContextUserMixin
        , public ComponentMixin<SpotLightComponent>
    {
    public:
        SpotLightImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(SpotLightImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"spot_light";
        }
    };

    class DirectionalLightImplementation
        : public ContextUserMixin
        , public ComponentMixin<DirectionalLightComponent>
    {
    public:
        DirectionalLightImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(DirectionalLightImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"directional_light";
        }
    };

    REGISTER_CLASS(PointLightImplementation)
    REGISTER_CLASS(SpotLightImplementation)
    REGISTER_CLASS(DirectionalLightImplementation)
}; // namespace Gek