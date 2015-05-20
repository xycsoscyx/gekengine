#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Engine\SystemInterface.h"

namespace Gek
{
    namespace Model
    {
        DECLARE_REGISTERED_CLASS(Component);
        namespace Component
        {
            DECLARE_INTERFACE_IID(Class, "9D68F36F-34C3-4E94-85F7-2420171BC5A9");
        }; // namespace Component

        DECLARE_REGISTERED_CLASS(System);
        namespace System
        {
            DECLARE_INTERFACE_IID(Class, "6AE951CC-3F57-45AF-A52A-29776E70CD22");
        }; // namespace System
    }; // namespace Model
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Model::Component::Class, Gek::Model::Component)
ADD_CLASS_TYPE(Gek::Engine::Component::Type)

ADD_CONTEXT_CLASS(Gek::Model::System::Class, Gek::Model::System)
    ADD_CLASS_TYPE(Gek::Engine::System::Type)
END_CONTEXT_SOURCE