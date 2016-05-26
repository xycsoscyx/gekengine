#include "GEK\Newton\StaticBody.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    StaticBodyComponent::StaticBodyComponent(void)
    {
    }

    HRESULT StaticBodyComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, shape);
        return S_OK;
    }

    HRESULT StaticBodyComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, shape);
        return S_OK;
    }

    class StaticBodyImplementation
        : public ContextUserMixin
        , public ComponentMixin<StaticBodyComponent>
    {
    public:
        StaticBodyImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(StaticBodyImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(const wchar_t *) getName(void) const
        {
            return L"static_body";
        }
    };

    REGISTER_CLASS(StaticBodyImplementation)
}; // namespace Gek