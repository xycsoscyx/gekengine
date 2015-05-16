#include "GEK\Newton\Player.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
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
                setComponentParameter(componentParameterList, L"outerRadius", outerRadius, String::getFloat);
                setComponentParameter(componentParameterList, L"innerRadius", innerRadius, String::getFloat);
                setComponentParameter(componentParameterList, L"height", height, String::getFloat);
                setComponentParameter(componentParameterList, L"stairStep", stairStep, String::getFloat);
                return S_OK;
            }

            class Component : public ContextUser
                            , public BaseComponent<Data>
            {
            public:
                Component(void)
                {
                }

                BEGIN_INTERFACE_LIST(Component)
                    INTERFACE_LIST_ENTRY_COM(ComponentInterface)
                END_INTERFACE_LIST_UNKNOWN

                // ComponentInterface
                STDMETHODIMP_(LPCWSTR) getName(void) const
                {
                    return L"Player";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return (Handle)&Identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Player
    } // namespace Components
}; // namespace Gek