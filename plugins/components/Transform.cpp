#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\BaseComponent.h"
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

                HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L"position"] = String::from(position);
                    componentParameterList[L"rotation"] = String::from(rotation);
                    return S_OK;
                }

                HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"position", position, String::to<Math::Float3>);
                    setParameter(componentParameterList, L"rotation", rotation, String::to<Math::Quaternion>);
                    return S_OK;
                }

                class Component : public ContextUserMixin
                    , public BaseComponent<Data>
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
                        return L"transform";
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Transform
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek