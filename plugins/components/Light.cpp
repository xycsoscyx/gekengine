#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    PointLightComponent::PointLightComponent(void)
    {
    }

    HRESULT PointLightComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"radius"] = String::from(radius);
        return S_OK;
    }

    HRESULT PointLightComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"radius", radius, String::to<float>);
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

    REGISTER_CLASS(PointLightImplementation)
}; // namespace Gek