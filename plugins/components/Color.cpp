#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ColorComponent::ColorComponent(void)
    {
    }

    HRESULT ColorComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        saveParameter(componentParameterList, L"", value);
        return S_OK;
    }

    HRESULT ColorComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        return (loadParameter(componentParameterList, L"", value) ? S_OK : E_INVALIDARG);
    }

    class ColorImplementation : public ContextUserMixin
        , public ComponentMixin<ColorComponent>
    {
    public:
        ColorImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ColorImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"color";
        }
    };

    REGISTER_CLASS(ColorImplementation)
}; // namespace Gek