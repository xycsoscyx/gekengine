#include "GEK\Components\Size.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        namespace Size
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
                setComponentParameter(componentParameterList, L"", value, String::getFloat);
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
                    return L"Size";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Size
    }; // namespace Components
}; // namespace Gek