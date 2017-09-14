#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Newton
    {
        GEK_CONTEXT_USER(Scene, Plugin::Population *)
            , public Plugin::ComponentMixin<Components::Scene>
        {
        public:
            Scene(Context *context, Plugin::Population *population)
                : ContextRegistration(context)
                , ComponentMixin(population)
            {
            }
        };

        GEK_REGISTER_CONTEXT_USER(Scene)
    }; // namespace Newton
}; // namespace Gek