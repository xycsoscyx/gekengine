#include "GEK\Components\Camera.h"
#include "GEK\Context\UserMixin.h"
#include "GEK\Engine\BaseComponent.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Camera
            {
                Data::Data(void)
                    : fieldOfView(Math::convertDegreesToRadians(90.0f))
                    , minimumDistance(1.0f)
                    , maximumDistance(100.0f)
                    , viewPort(0.0f, 0.0f, 1.0f, 1.0f)
                {
                }

                HRESULT Data::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L"field_of_view"] = String::from(fieldOfView);
                    componentParameterList[L"minimum_distance"] = String::from(minimumDistance);
                    componentParameterList[L"maximum_distance"] = String::from(maximumDistance);
                    componentParameterList[L"viewport"] = String::from(viewPort);
                    return S_OK;
                }

                HRESULT Data::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"field_of_view", fieldOfView, String::toFloat);
                    setParameter(componentParameterList, L"minimum_distance", minimumDistance, String::toFloat);
                    setParameter(componentParameterList, L"maximum_distance", maximumDistance, String::toFloat);
                    setParameter(componentParameterList, L"viewport", viewPort, String::toFloat4);
                    return S_OK;
                }

                class Component : public Context::User::Mixin
                    , public BaseComponent< Data >
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
                        return L"camera";
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Camera
        } // namespace Components
    }; // namespace Engine
}; // namespace Gek