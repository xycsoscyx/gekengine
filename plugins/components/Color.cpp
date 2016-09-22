#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Components
    {
        void Color::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, value);
        }

        void Color::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            value = loadParameter(componentData, nullptr, Math::Color::White);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Color)
        , public Plugin::ComponentMixin<Components::Color>
    {
    public:
        Color(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"color";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek