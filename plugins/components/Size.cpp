#include "GEK\Components\Size.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Size
            {
                Data::Data(void)
                    : value(0.0f)
                {
                }

                HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L""] = String::from(value);
                    return S_OK;
                }

                HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"", value, String::toFloat);
                    return S_OK;
                }

                class Component : public Context::User::Mixin
                    , public BaseComponent< Data >
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
                        return L"size";
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Size
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek