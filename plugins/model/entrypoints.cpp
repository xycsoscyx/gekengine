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
        DECLARE_INTERFACE_IID(Class, "9D68F36F-34C3-4E94-85F7-2420171BC5A9");

        DECLARE_REGISTERED_CLASS(System);
        //DECLARE_INTERFACE_IID(Class, "975839FD-6605-4BDB-AB29-ADE09CA0BC91");
    }; // namespace Model
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Model::Class, Gek::Model::Component)
ADD_CLASS_TYPE(Gek::Engine::Component::Type)

ADD_CONTEXT_CLASS(Gek::Model::Class, Gek::Model::System)
    ADD_CLASS_TYPE(Gek::Engine::System::Type)
END_CONTEXT_SOURCE