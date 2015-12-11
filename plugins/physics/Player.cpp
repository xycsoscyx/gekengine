#include "GEK\Newton\Player.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    PlayerComponent::PlayerComponent(void)
        : outerRadius(1.0f)
        , innerRadius(0.25f)
        , height(1.9f)
        , stairStep(0.25f)
    {
    }

    HRESULT PlayerComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        componentParameterList[L"outer_radius"] = String::from(outerRadius);
        componentParameterList[L"inner_radius"] = String::from(innerRadius);
        componentParameterList[L"height"] = String::from(height);
        componentParameterList[L"stair_step"] = String::from(stairStep);
        return S_OK;
    }

    HRESULT PlayerComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"outer_radius", outerRadius, String::to<float>);
        setParameter(componentParameterList, L"inner_radius", innerRadius, String::to<float>);
        setParameter(componentParameterList, L"height", height, String::to<float>);
        setParameter(componentParameterList, L"stair_step", stairStep, String::to<float>);
        return S_OK;
    }

    class PlayerImplementation : public ContextUserMixin
        , public ComponentMixin<PlayerComponent>
    {
    public:
        PlayerImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(PlayerImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"player_body";
        }
    };

    REGISTER_CLASS(PlayerImplementation)
}; // namespace Gek