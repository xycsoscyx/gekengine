#include "GEK\Components\Size.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    SizeComponent::SizeComponent(void)
        : value(0.0f)
    {
    }

    HRESULT SizeComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = String::from(value);
        return S_OK;
    }

    HRESULT SizeComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", value, String::to<float>);
        return S_OK;
    }

    class SizeImplementation : public ContextUserMixin
        , public ComponentMixin<SizeComponent>
    {
    public:
        SizeImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(SizeImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component::Interface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"size";
        }
    };

    REGISTER_CLASS(SizeImplementation)
}; // namespace Gek