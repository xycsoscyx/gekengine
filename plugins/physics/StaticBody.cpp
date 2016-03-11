#include "GEK\Newton\StaticBody.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    StaticBodyComponent::StaticBodyComponent(void)
    {
    }

    HRESULT StaticBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        getParameter(componentParameterList, L"", shape);
        return S_OK;
    }

    HRESULT StaticBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", shape);
        return S_OK;
    }

    class StaticBodyImplementation : public ContextUserMixin
        , public ComponentMixin<StaticBodyComponent>
    {
    public:
        StaticBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(StaticBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"static_body";
        }
    };

    REGISTER_CLASS(StaticBodyImplementation)
}; // namespace Gek