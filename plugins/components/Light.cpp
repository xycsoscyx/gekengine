#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    PointLightComponent::PointLightComponent(void)
    {
    }

    HRESULT PointLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        saveParameter(componentParameterList, L"range", range);
        saveParameter(componentParameterList, L"radius", radius);
        return S_OK;
    }

    HRESULT PointLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        return (loadParameter(componentParameterList, L"range", range) &&
            loadParameter(componentParameterList, L"radius", radius) ? S_OK : E_INVALIDARG);
    }

    SpotLightComponent::SpotLightComponent(void)
    {
    }

    HRESULT SpotLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        saveParameter(componentParameterList, L"range", range);
        saveParameter(componentParameterList, L"radius", radius);
        saveParameter(componentParameterList, L"inner_angle", innerAngle);
        saveParameter(componentParameterList, L"outer_angle", outerAngle);
        return S_OK;
    }

    HRESULT SpotLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        return (loadParameter(componentParameterList, L"range", range) &&
            loadParameter(componentParameterList, L"radius", radius) &&
            loadParameter(componentParameterList, L"inner_angle", innerAngle) &&
            loadParameter(componentParameterList, L"outer_angle", outerAngle) ? S_OK : E_INVALIDARG);
    }

    DirectionalLightComponent::DirectionalLightComponent(void)
    {
    }

    HRESULT DirectionalLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        return E_NOTIMPL;
    }

    HRESULT DirectionalLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        return S_OK;
    }

    class PointLightImplementation : public ContextUserMixin
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

    class SpotLightImplementation : public ContextUserMixin
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

    class DirectionalLightImplementation : public ContextUserMixin
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