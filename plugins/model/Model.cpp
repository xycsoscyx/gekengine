#include "GEK\Engine\Model.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Model
    {
        Data::Data(void)
        {
        }

        HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L""] = value;
            return S_OK;
        }

        HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            Engine::setParameter(componentParameterList, L"", value, [](LPCWSTR value) -> LPCWSTR { return value; });
            return S_OK;
        }

        class Component : public Context::User::Mixin
            , public Engine::BaseComponent< Data >
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
                return L"model";
            }
        };

        REGISTER_CLASS(Component)
    }; // namespace Model
}; // namespace Gek