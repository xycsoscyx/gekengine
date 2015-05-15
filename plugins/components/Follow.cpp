#include "GEK\Components\Follow.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Follow::Follow(void)
        {
        }

        HRESULT Follow::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L""] = target;
            componentParameterList[L"offset"] = String::setFloat3(offset);
            componentParameterList[L"rotation"] = String::setQuaternion(rotation);
            return S_OK;
        }

        HRESULT Follow::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            setComponentParameter(componentParameterList, L"", target, [] (LPCWSTR value) -> LPCWSTR { return value; });
            setComponentParameter(componentParameterList, L"offset", offset, String::getFloat3);
            setComponentParameter(componentParameterList, L"rotation", rotation, String::getQuaternion);
            return S_OK;
        }
    }; // namespace Components

    class FollowComponent : public ContextUser
                          , public BaseComponent<Gek::Components::Follow>
    {
    public:
        FollowComponent(void)
        {
        }

        BEGIN_INTERFACE_LIST(FollowComponent)
            INTERFACE_LIST_ENTRY_COM(ComponentInterface)
        END_INTERFACE_LIST_UNKNOWN

        // ComponentInterface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"Follow";
        }

        STDMETHODIMP_(Handle) getID(void) const
        {
            return 0;
        }
    };

    REGISTER_CLASS(FollowComponent)
}; // namespace Gek}