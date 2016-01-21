#include "GEK\Components\Follow.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\Evaluator.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    FollowComponent::FollowComponent(void)
        : speed(1.0f)
    {
    }

    HRESULT FollowComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L""] = target;
        componentParameterList[L"mode"] = mode;
        componentParameterList[L"distance"] = String::from(distance);
        componentParameterList[L"speed"] = String::from(speed);
        return S_OK;
    }

    HRESULT FollowComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"", target, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"mode", mode, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"distance", distance, Evaluator::get<Math::Float3>);
        setParameter(componentParameterList, L"speed", speed, Evaluator::get<float>);
        return S_OK;
    }

    class FollowImplementation : public ContextUserMixin
        , public ComponentMixin<FollowComponent>
    {
    public:
        FollowImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(FollowImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"follow";
        }
    };

    REGISTER_CLASS(FollowImplementation)
}; // name