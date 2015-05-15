#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Light::Light(void)
        {
        }

        HRESULT Light::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L"radius"] = String::setFloat(radius);
            return S_OK;
        }

        HRESULT Light::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            setComponentParameter(componentParameterList, L"radius", radius, String::getFloat);
            return S_OK;
        }
    }; // namespace Components

    class LightComponent : public ContextUser
                          , public BaseComponent<Gek::Components::Light>
    {
    public:
        LightComponent(void)
        {
        }

        BEGIN_INTERFACE_LIST(LightComponent)
            INTERFACE_LIST_ENTRY_COM(ComponentInterface)
        END_INTERFACE_LIST_UNKNOWN

        // ComponentInterface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"Light";
        }

        STDMETHODIMP_(Handle) getID(void) const
        {
            return 0;
        }
    };

    REGISTER_CLASS(LightComponent)
}; // namespace Gek