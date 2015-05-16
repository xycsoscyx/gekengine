#include "GEK\Components\Camera.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        namespace Camera
        {
            Data::Data(void)
                : fieldOfView(Math::convertDegreesToRadians(90.0f))
                , minimumDistance(0.1f)
                , maximumDistance(100.0f)
                , viewPort(0.0f, 0.0f, 1.0f, 1.0f)
            {
            }

            HRESULT Data::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
            {
                componentParameterList[L"fieldOfView"] = String::setFloat(fieldOfView);
                componentParameterList[L"minimumDistance"] = String::setFloat(minimumDistance);
                componentParameterList[L"maximumDistance"] = String::setFloat(maximumDistance);
                componentParameterList[L"fieldOfView"] = String::setFloat4(viewPort);
                return S_OK;
            }

            HRESULT Data::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
            {
                setComponentParameter(componentParameterList, L"fieldOfView", fieldOfView, String::getFloat);
                setComponentParameter(componentParameterList, L"minimumDistance", minimumDistance, String::getFloat);
                setComponentParameter(componentParameterList, L"maximumDistance", maximumDistance, String::getFloat);
                setComponentParameter(componentParameterList, L"viewPort", viewPort, String::getFloat4);
                return S_OK;
            }

            class Component : public ContextUser
                            , public BaseComponent<Data>
            {
            public:
                Component(void)
                {
                }

                BEGIN_INTERFACE_LIST(Component)
                    INTERFACE_LIST_ENTRY_COM(ComponentInterface)
                END_INTERFACE_LIST_UNKNOWN

                // ComponentInterface
                STDMETHODIMP_(LPCWSTR) getName(void) const
                {
                    return L"Camera";
                }

                STDMETHODIMP_(Handle) getIdentifier(void) const
                {
                    return (Handle)&Identifier;
                }
            };

            REGISTER_CLASS(Component)
        }; // namespace Camera
    } // namespace Components
}; // namespace Gek