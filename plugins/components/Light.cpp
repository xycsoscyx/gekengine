#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    PointLightComponent::PointLightComponent(void)
        : range(0.0f)
        , radius(0.0f)
    {
    }

    HRESULT PointLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"range"] = String::from(range);
        componentParameterList[L"radius"] = String::from(radius);
        return S_OK;
    }

    HRESULT PointLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"range", range, Evaluator::get<float>);
        setParameter(componentParameterList, L"radius", radius, Evaluator::get<float>);
        return S_OK;
    }

    SpotLightComponent::SpotLightComponent(void)
        : range(0.0f)
        , radius(0.0f)
        , innerAngle(0.0f)
        , outerAngle(0.0f)
    {
    }

    HRESULT SpotLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"range"] = String::from(range);
        componentParameterList[L"radius"] = String::from(radius);
        componentParameterList[L"inner_angle"] = String::from(innerAngle);
        componentParameterList[L"outer_angle"] = String::from(outerAngle);
        return S_OK;
    }

    HRESULT SpotLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"range", range, Evaluator::get<float>);
        setParameter(componentParameterList, L"radius", radius, Evaluator::get<float>);
        setParameter(componentParameterList, L"inner_angle", innerAngle, Evaluator::get<float>);
        setParameter(componentParameterList, L"outer_angle", outerAngle, Evaluator::get<float>);
        return S_OK;
    }

    DirectionalLightComponent::DirectionalLightComponent(void)
    {
    }

    HRESULT DirectionalLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        return S_OK;
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