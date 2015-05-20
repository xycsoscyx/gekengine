#include "GEK\Newton\Player.h"
#include "GEK\Context\BaseUser.h"
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

            HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L"outerRadius"] = String::setFloat(outerRadius);
                componentParameterList[L"innerRadius"] = String::setFloat(innerRadius);
                componentParameterList[L"height"] = String::setFloat(height);
                componentParameterList[L"stairStep"] = String::setFloat(stairStep);
                return S_OK;
            }

            HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                Engine::setParameter(componentParameterList, L"outerRadius", outerRadius, String::getFloat);
                Engine::setParameter(componentParameterList, L"innerRadius", innerRadius, String::getFloat);
                Engine::setParameter(componentParameterList, L"height", height, String::getFloat);
                Engine::setParameter(componentParameterList, L"stairStep", stairStep, String::getFloat);
                return S_OK;
            }

            class Component : public Context::BaseUser
                , public Engine::BaseComponent<Data>
            {
            public:
                Component(void)
                {
                }

                BEGIN_INTERFACE_LIST(Component)
                    INTERFACE_LIST_ENTRY_COM(Component::Interface)
                    END_INTERFACE_LIST_USER

                // Component::Interface
                STDMETHODIMP_(LPCWSTR) getName(void) const
                {
                    return L"Player";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Player
    }; // namespace Newton
}; // namespace Gek