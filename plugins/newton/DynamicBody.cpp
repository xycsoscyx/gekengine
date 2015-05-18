#include "GEK\Newton\DynamicBody.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Newton
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
                    Engine::setParameter(componentParameterList, L"", shape, [](LPCWSTR value) -> LPCWSTR { return value; });
                    Engine::setParameter(componentParameterList, L"material", material, [](LPCWSTR value) -> LPCWSTR { return value; });
                    return S_OK;
                }

                class Component : public Context::BaseUser
                    , public Engine::BaseComponent<Data>
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
                        return L"DynamicBody";
                    }

                    STDMETHODIMP_(Handle) getIdentifier(void) const
                    {
                        return identifier;
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace DynamicBody
        }; // namespace Newton
    } // namespace Components
}; // namespace Gek