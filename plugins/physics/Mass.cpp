#include "GEK\Newton\Mass.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        Mass::Mass(void)
        {
        }

        void Mass::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, value);
        }

        void Mass::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            value = loadParameter(componentData, nullptr, 0.0f);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Mass)
        , public Plugin::ComponentMixin<Components::Mass>
    {
    public:
        Mass(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"mass";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Mass)
}; // namespace Gek