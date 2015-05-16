#include "GEK\Newton\DynamicBody.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        namespace DynamicBody
        {
            Data::Data(void)
            {
            }

            HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L""] = shape;
                componentParameterList[L"material"] = material;
                return S_OK;
            }

            HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                setComponentParameter(componentParameterList, L"", shape, [](LPCWSTR value) -> LPCWSTR { return value; });
                setComponentParameter(componentParameterList, L"material", material, [](LPCWSTR value) -> LPCWSTR { return value; });
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
                    return L"DynamicBody";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return (Handle)&Identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace DynamicBody
    } // namespace Components
}; // namespace Gek