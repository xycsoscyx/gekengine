#include "GEK\Newton\Mass.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Newton
    {
        namespace Mass
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
                Engine::setParameter(componentParameterList, L"", value, String::to<float>);
                return S_OK;
            }

            class Component : public Context::User::Mixin
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
                    return L"mass";
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Mass
    }; // namespace Newton
}; // namespace Gek