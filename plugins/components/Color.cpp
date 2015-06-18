#include "GEK\Components\Color.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Engine
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
                    setParameter(componentParameterList, L"", value, String::getFloat4);
                    return S_OK;
                }

                class Component : public Context::BaseUser
                    , public BaseComponent < Data >
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
                        return L"color";
                    }

                    STDMETHODIMP_(UINT32) getIdentifier(void) const
                    {
                        return identifier;
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Color
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek