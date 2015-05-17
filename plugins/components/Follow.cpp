#include "GEK\Components\Follow.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Follow
            {
                Data::Data(void)
                {
                }

                HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L""] = target;
                    componentParameterList[L"offset"] = String::setFloat3(offset);
                    componentParameterList[L"rotation"] = String::setQuaternion(rotation);
                    return S_OK;
                }

                HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"", target, [](LPCWSTR value) -> LPCWSTR { return value; });
                    setParameter(componentParameterList, L"offset", offset, String::getFloat3);
                    setParameter(componentParameterList, L"rotation", rotation, String::getQuaternion);
                    return S_OK;
                }

                class Component : public ContextUser
                    , public BaseComponent < Data >
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
                        return L"Follow";
                    }

                    STDMETHODIMP_(Handle) getIdentifier(void) const
                    {
                        return identifier;
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Follow
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek