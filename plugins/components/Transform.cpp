#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Transform::Transform(void)
        {
        }

        HRESULT Transform::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L"position"] = String::setFloat3(position);
            componentParameterList[L"rotation"] = String::setQuaternion(rotation);
            return S_OK;
        }

        HRESULT Transform::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            setComponentParameter(componentParameterList, L"position", position, String::getFloat3);
            setComponentParameter(componentParameterList, L"rotation", rotation, String::getQuaternion);
            return S_OK;
        }
    }; // namespace Components

    class TransformComponent : public ContextUser
        , public BaseComponent < Gek::Components::Transform >
    {
    public:
        TransformComponent(void)
        {
        }

        BEGIN_INTERFACE_LIST(TransformComponent)
            INTERFACE_LIST_ENTRY_COM(ComponentInterface)
            END_INTERFACE_LIST_UNKNOWN

            // ComponentInterface
            STDMETHODIMP_(LPCWSTR) getName(void) const
            {
                return L"Transform";
            }

            STDMETHODIMP_(Handle) getID(void) const
            {
                return 0;
            }
    };

    REGISTER_CLASS(TransformComponent)
}; // namespace Gek