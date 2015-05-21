#include "GEK\Engine\CoreInterface.h"
#include "GEK\Engine\PopulationInterface.h"
#include "GEK\Engine\RenderInterface.h"
#include "GEK\Engine\SystemInterface.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\XML.h"
#include <set>
#include <concurrent_vector.h>
#include <concurrent_unordered_map.h>
#include <ppl.h>

namespace Gek
{
    namespace Engine
    {
        namespace Core
        {
            class System : public Context::BaseUser
                , public Interface
            {
            private:

            public:
                System(void)
                {
                }

                ~System(void)
                {
                }

                BEGIN_INTERFACE_LIST(System)
                    INTERFACE_LIST_ENTRY_COM(Interface)
                END_INTERFACE_LIST_USER
            };

            REGISTER_CLASS(System)
        }; // namespace Core
    }; // namespace Engine
}; // namespace Gek
