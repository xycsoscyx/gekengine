#include "GEK\Newton\Mass.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        namespace Mass
        {
            Data::Data(void)
                : value(0.0f)
            {
            }

            HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L""] = String::setFloat(value);
                return S_OK;
            }

            HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                setParameter(componentParameterList, L"", value, String::getFloat);
                return S_OK;
            }

            class Component : public Context::BaseUser
                            , public BaseComponent<Data>
            {
            public:
                Component(void)
                {
                }

                BEGIN_INTERFACE_LIST(Component)
                    INTERFACE_LIST_ENTRY_COM(Component::Interface)
                END_INTERFACE_LIST_UNKNOWN

                // Component::Interface
                STDMETHODIMP_(LPCWSTR) getName(void) const
                {
                    return L"Mass";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Mass
    } // namespace Components
}; // namespace Gek