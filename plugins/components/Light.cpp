#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace PointLight
            {
                Data::Data(void)
                {
                }

                HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L"radius"] = String::from(radius);
                    return S_OK;
                }

                HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"radius", radius, String::to<float>);
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
                        return L"point_light";
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Light
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek