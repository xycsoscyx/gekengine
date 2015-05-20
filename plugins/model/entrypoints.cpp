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
                DECLARE_INTERFACE_IID(Class, "06FD2921-89A3-490C-BBF6-9C909D921E96");
            }; // namespace Model
        }; // namespace Components
        
        namespace Model
        {
            DECLARE_REGISTERED_CLASS(System);
            DECLARE_INTERFACE_IID(Class, "BCF7CC29-F375-4F29-9DA7-921F6A78E4E3");
        }; // namespace Model
    }; // namespace Engine
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Engine::Components::Model::Class, Gek::Engine::Components::Model::Component)
ADD_CLASS_TYPE(Gek::Engine::Component::Type)

ADD_CONTEXT_CLASS(Gek::Engine::Model::Class, Gek::Engine::Model::System)
    ADD_CLASS_TYPE(Gek::Engine::System::Type)
END_CONTEXT_SOURCE