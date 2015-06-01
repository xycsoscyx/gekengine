#include "GEK\Components\Camera.h"
#include "GEK\Context\BaseUser.h"
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

                HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
                {
                    componentParameterList[L"field_of_view"] = String::setFloat(fieldOfView);
                    componentParameterList[L"minimum_distance"] = String::setFloat(minimumDistance);
                    componentParameterList[L"maximum_distance"] = String::setFloat(maximumDistance);
                    componentParameterList[L"viewport"] = String::setFloat4(viewPort);
                    return S_OK;
                }

                HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
                {
                    setParameter(componentParameterList, L"field_of_view", fieldOfView, String::getFloat);
                    setParameter(componentParameterList, L"minimum_distance", minimumDistance, String::getFloat);
                    setParameter(componentParameterList, L"maximum_distance", maximumDistance, String::getFloat);
                    setParameter(componentParameterList, L"viewport", viewPort, String::getFloat4);
                    return S_OK;
                }

                class Component : public Context::BaseUser
                    , public BaseComponent < Data >
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
                        return L"camera";
                    }

                    STDMETHODIMP_(Handle) getIdentifier(void) const
                    {
                        return identifier;
                    }
                };

                REGISTER_CLASS(Component)
            }; // namespace Camera
        } // namespace Components
    }; // namespace Engine
}; // namespace Gek