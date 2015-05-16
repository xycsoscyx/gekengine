#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        namespace Color
        {
            Data::Data(void)
                : value(1.0f, 1.0f, 1.0f, 1.0f)
            {
            }

            HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L""] = String::setFloat4(value);
                return S_OK;
            }

            HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                setComponentParameter(componentParameterList, L"", value, String::getFloat4);
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
                    return L"Color";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Color
    }; // namespace Components
}; // namespace Gek