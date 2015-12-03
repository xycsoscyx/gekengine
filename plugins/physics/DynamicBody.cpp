#include "GEK\Newton\DynamicBody.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    DynamicBodyComponent::DynamicBodyComponent(void)
    {
    }

    HRESULT DynamicBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = shape;
        componentParameterList[L"surface"] = surface;
        return S_OK;
    }

    HRESULT DynamicBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", shape, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"surface", surface, [](LPCWSTR value) -> LPCWSTR { return value; });
        return S_OK;
    }

    class DynamicBodyImplementation : public ContextUserMixin
        , public ComponentMixin<DynamicBodyComponent>
    {
    public:
        DynamicBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(DynamicBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component::Interface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"dynamic_body";
        }
    };

    REGISTER_CLASS(DynamicBodyImplementation)
}; // namespace Gek