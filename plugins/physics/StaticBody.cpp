#include "GEK\Newton\StaticBody.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        StaticBody::StaticBody(void)
        {
        }

        void StaticBody::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, shape);
        }

        void StaticBody::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            shape = loadParameter(componentData, nullptr, String());
        }
    }; // namespace Components

    GEK_CONTEXT_USER(StaticBody)
        , public Plugin::ComponentMixin<Components::StaticBody>
    {
    public:
        StaticBody(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"static_body";
        }
    };

    GEK_REGISTER_CONTEXT_USER(StaticBody)
}; // namespace Gek