#include "GEK\Components\Follow.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\Engine\BaseComponent.h"
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
                    componentParameterList[L"offset"] = String::from(offset);
                    componentParameterList[L"rotation"] = String::from(rotation);
                    return S_OK;
                }

                HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"", target, [](LPCWSTR value) -> LPCWSTR { return value; });
                    setParameter(componentParameterList, L"offset", offset, String::toFloat4);
                    setParameter(componentParameterList, L"rotation", rotation, String::toQuaternion);
                    return S_OK;
                }

                class Component : public Context::User::Mixin
                    , public BaseComponent< Data, AlignedAllocator<Data, 16> >
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
                        return L"follower";
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Follow
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek