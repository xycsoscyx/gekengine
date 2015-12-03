#include "GEK\Engine\Model.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ModelComponent::ModelComponent(void)
    {
    }

    HRESULT ModelComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = value;
        return S_OK;
    }

    HRESULT ModelComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", value, [](LPCWSTR value) -> LPCWSTR { return value; });
        return S_OK;
    }

    class ModelImplementation : public ContextUserMixin
        , public ComponentMixin<ModelComponent>
    {
    public:
        ModelImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ModelImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component::Interface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"model";
        }
    };

    REGISTER_CLASS(ModelImplementation)
}; // namespace Gek