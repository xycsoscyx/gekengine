#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Engine\SystemInterface.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Model
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "9D68F36F-34C3-4E94-85F7-2420171BC5A9");
            }; // namespace Model
        }; // namespace Components
        
        namespace Model
        {
            DECLARE_REGISTERED_CLASS(System);
            DECLARE_INTERFACE_IID(Class, "975839FD-6605-4BDB-AB29-ADE09CA0BC91");
        }; // namespace Model
    }; // namespace Engine
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Engine::Components::Model::Class, Gek::Engine::Components::Model::Component)
ADD_CLASS_TYPE(Gek::Engine::Component::Type)

ADD_CONTEXT_CLASS(Gek::Engine::Model::Class, Gek::Engine::Model::System)
    ADD_CLASS_TYPE(Gek::Engine::System::Type)
END_CONTEXT_SOURCE