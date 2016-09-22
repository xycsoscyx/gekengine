#include "GEK\Newton\Base.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        void Physical::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"mass", mass);
        }

        void Physical::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            mass = loadParameter(componentData, L"mass", 0.0f);
        }
    }; // namespace Components

    namespace Newton
    {
        GEK_CONTEXT_USER(Physical)
            , public Plugin::ComponentMixin<Components::Physical>
        {
        public:
            Physical(Context *context)
                : ContextRegistration(context)
            {
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"physical";
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek