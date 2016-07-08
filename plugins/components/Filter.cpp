#include "GEK\Components\Filter.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        Filter::Filter(void)
        {
        }

        void Filter::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, String(list, L','));
        }

        void Filter::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            list = loadParameter(componentData, nullptr, String()).split(L',');
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Filter)
        , public Plugin::ComponentMixin<Components::Filter>
    {
    public:
        Filter(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"filter";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Filter);
}; // namespace Gek