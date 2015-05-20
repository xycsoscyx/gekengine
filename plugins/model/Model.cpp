#include "GEK\Engine\Model.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Model
    {
        Data::Data(void)
        {
        }

        HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L""] = value;
            return S_OK;
        }

        HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            Engine::setParameter(componentParameterList, L"", value, [](LPCWSTR value) -> LPCWSTR { return value; });
            return S_OK;
        }

        class Component : public Context::BaseUser
            , public Engine::BaseComponent < Data >
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
                return L"Model";
            }

            STDMETHODIMP_(Handle) getIdentifier(void) const
            {
                return identifier;
            }
        };

        REGISTER_CLASS(Component)
    }; // namespace Model
}; // namespace Gek