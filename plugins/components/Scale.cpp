#include "GEK\Components\Scale.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ScaleComponent::ScaleComponent(void)
        : value(0.0f)
    {
    }

    HRESULT ScaleComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = String::from(value);
        return S_OK;
    }

    HRESULT ScaleComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", value, String::to<Math::Float3>);
        return S_OK;
    }

    class ScaleImplementation : public ContextUserMixin
        , public ComponentMixin<ScaleComponent>
    {
    public:
        ScaleImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ScaleImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"scale";
        }
    };

    REGISTER_CLASS(ScaleImplementation)
}; // namespace Gek