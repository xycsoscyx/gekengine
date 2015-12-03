#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    TransformComponent::TransformComponent(void)
    {
    }

    HRESULT TransformComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"position"] = String::from(position);
        componentParameterList[L"rotation"] = String::from(rotation);
        return S_OK;
    }

    HRESULT TransformComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"position", position, String::to<Math::Float3>);
        setParameter(componentParameterList, L"rotation", rotation, String::to<Math::Quaternion>);
        return S_OK;
    }

    class TransformImplementation : public ContextUserMixin
        , public ComponentMixin<TransformComponent>
    {
    public:
        TransformImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(TransformImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
            END_INTERFACE_LIST_USER

        // Component::Interface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"transform";
        }
    };

    REGISTER_CLASS(TransformImplementation)
}; // namespace Gek