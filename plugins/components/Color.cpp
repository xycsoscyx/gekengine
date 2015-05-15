#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Color::Color(void)
            : value(1.0f, 1.0f, 1.0f, 1.0f)
        {
        }

        HRESULT Color::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L""] = String::setFloat4(value);
            return S_OK;
        }

        HRESULT Color::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            setComponentParameter(componentParameterList, L"", value, String::getFloat4);
            return S_OK;
        }
    }; // namespace Components

    class ColorComponent : public ContextUser
                          , public BaseComponent<Gek::Components::Color>
    {
    public:
        ColorComponent(void)
        {
        }

        BEGIN_INTERFACE_LIST(ColorComponent)
            INTERFACE_LIST_ENTRY_COM(ComponentInterface)
        END_INTERFACE_LIST_UNKNOWN

        // ComponentInterface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"Color";
        }

        STDMETHODIMP_(Handle) getID(void) const
        {
            return 0;
        }
    };

    REGISTER_CLASS(ColorComponent)
}; // namespace Gek