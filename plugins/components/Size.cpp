#include "GEK\Components\Size.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Component.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Size::Size(void)
            : value(0.0f)
        {
        }

        HRESULT Size::getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const
        {
            componentParameterList[L""] = String::setFloat(value);
            return S_OK;
        }

        HRESULT Size::setData(const std::unordered_map<CStringW, CStringW> &componentParameterList)
        {
            setComponentParameter(componentParameterList, L"", value, String::getFloat);
            return S_OK;
        }
    }; // namespace Components

    class SizeComponent : public ContextUser
                          , public BaseComponent<Gek::Components::Size>
    {
    public:
        SizeComponent(void)
        {
        }

        BEGIN_INTERFACE_LIST(SizeComponent)
            INTERFACE_LIST_ENTRY_COM(ComponentInterface)
        END_INTERFACE_LIST_UNKNOWN

        // ComponentInterface
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"Size";
        }

        STDMETHODIMP_(Handle) getID(void) const
        {
            return 0;
        }
    };

    REGISTER_CLASS(SizeComponent)
}; // namespace Gek