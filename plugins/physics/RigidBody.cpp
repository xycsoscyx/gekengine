#include "GEK\Newton\RigidBody.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    RigidBodyComponent::RigidBodyComponent(void)
    {
    }

    HRESULT RigidBodyComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = shape;
        componentParameterList[L"surface"] = surface;
        return S_OK;
    }

    HRESULT RigidBodyComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", shape, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"surface", surface, [](LPCWSTR value) -> LPCWSTR { return value; });
        return S_OK;
    }

    class RigidBodyImplementation : public ContextUserMixin
        , public ComponentMixin<RigidBodyComponent>
    {
    public:
        RigidBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(RigidBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"rigid_body";
        }
    };

    REGISTER_CLASS(RigidBodyImplementation)
}; // namespace Gek