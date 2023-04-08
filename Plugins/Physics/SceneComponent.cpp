#include "GEK/Physics/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Physics
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
    }; // namespace Physics
}; // namespace Gek