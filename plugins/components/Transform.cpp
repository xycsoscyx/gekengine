#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Transform
            {
                Data::Data(void)
                {
                }

                HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L"position"] = String::setFloat3(position);
                    componentParameterList[L"rotation"] = String::setQuaternion(rotation);
                    return S_OK;
                }

                HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"position", position, String::getFloat3);
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
                        return L"Transform";
                    }

                    STDMETHODIMP_(Handle) getIdentifier(void) const
                    {
                        return identifier;
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Transform
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek