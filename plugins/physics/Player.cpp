#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\Base.h"
#include <Newton.h>
#include <algorithm>
#include <memory>
#include <stack>

namespace Gek
{
    namespace Components
    {
        Player::Player(void)
        {
        }

        void Player::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"height", height);
            saveParameter(componentData, L"outer_radius", outerRadius);
            saveParameter(componentData, L"inner_radius", innerRadius);
            saveParameter(componentData, L"stair_step", stairStep);
        }

        void Player::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            height = loadParameter(componentData, L"height", 6.0f);
            outerRadius = loadParameter(componentData, L"outer_radius", 1.5f);
            innerRadius = loadParameter(componentData, L"inner_radius", 0.5f);
            stairStep = loadParameter(componentData, L"stair_step", 1.5f);
        }
    }; // namespace Components

    namespace Newton
    {
        GEK_CONTEXT_USER(Player)
            , public Plugin::ComponentMixin<Components::Player>
        {
        public:
            Player(Context *context)
                : ContextRegistration(context)
            {
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"player";
            }
        };

        GEK_REGISTER_CONTEXT_USER(Player)
    }; // namespace Newton
}; // namespace Gek