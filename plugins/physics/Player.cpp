#include "GEK\Newton\Player.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Newton
    {
        namespace Player
        {
            Data::Data(void)
                : outerRadius(1.0f)
                , innerRadius(0.25f)
                , height(1.9f)
                , stairStep(0.25f)
            {
            }

            HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L"outer_radius"] = String::from(outerRadius);
                componentParameterList[L"inner_radius"] = String::from(innerRadius);
                componentParameterList[L"height"] = String::from(height);
                componentParameterList[L"stair_step"] = String::from(stairStep);
                return S_OK;
            }

            HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                Engine::setParameter(componentParameterList, L"outer_radius", outerRadius, String::to<float>);
                Engine::setParameter(componentParameterList, L"inner_radius", innerRadius, String::to<float>);
                Engine::setParameter(componentParameterList, L"height", height, String::to<float>);
                Engine::setParameter(componentParameterList, L"stair_step", stairStep, String::to<float>);
                return S_OK;
            }

            class Component : public ContextUserMixin
                , public Engine::BaseComponent<Data>
            {
            public:
                Component(void)
                {
                }

                BEGIN_INTERFACE_LIST(Component)
                    INTERFACE_LIST_ENTRY_COM(Engine::Component::Interface)
                    END_INTERFACE_LIST_USER

                // Component::Interface
                STDMETHODIMP_(LPCWSTR) getName(void) const
                {
                    return L"player_body";
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Player
    }; // namespace Newton
}; // namespace Gek