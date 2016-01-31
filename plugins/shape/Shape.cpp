#include "GEK\Engine\Shape.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ShapeComponent::ShapeComponent(void)
    {
    }

    HRESULT ShapeComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = value;
        componentParameterList[L"parameters"] = parameters;
        componentParameterList[L"skin"] = skin;
        return S_OK;
    }

    HRESULT ShapeComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", value, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"parameters", parameters, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"skin", skin, [](LPCWSTR value) -> LPCWSTR { return value; });
        return S_OK;
    }

    class ShapeImplementation : public ContextUserMixin
        , public ComponentMixin<ShapeComponent>
    {
    public:
        ShapeImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ShapeImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"shape";
        }
    };

    REGISTER_CLASS(ShapeImplementation)
}; // namespace Gek