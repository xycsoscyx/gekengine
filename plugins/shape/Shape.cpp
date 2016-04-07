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
        saveParameter(componentParameterList, L"", value);
        saveParameter(componentParameterList, L"parameters", parameters);
        saveParameter(componentParameterList, L"skin", skin);
        return S_OK;
    }

    HRESULT ShapeComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        loadParameter(componentParameterList, L"", value);
        loadParameter(componentParameterList, L"parameters", parameters);
        loadParameter(componentParameterList, L"skin", skin);
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