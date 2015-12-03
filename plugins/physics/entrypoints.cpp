#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Engine\SystemInterface.h"

namespace Gek
{
    namespace Newton
    {
        namespace Mass
        {
            DECLARE_REGISTERED_CLASS(Component);
            DECLARE_INTERFACE_IID(Class, "06FD2921-89A3-490C-BBF6-9C909D921E96");
        }; // namespace Mass

        namespace DynamicBody
        {
            DECLARE_REGISTERED_CLASS(Component);
            DECLARE_INTERFACE_IID(Class, "5C52FBF3-261D-4016-9881-80E418C8E639");
        }; // namespace DynamicBody

        namespace Player
        {
            DECLARE_REGISTERED_CLASS(Component);
            DECLARE_INTERFACE_IID(Class, "EF85AE66-9544-4B7C-A2CF-B2410BDA63BD");
        }; // namespace Player

        DECLARE_REGISTERED_CLASS(System);
        DECLARE_INTERFACE_IID(Class, "BCF7CC29-F375-4F29-9DA7-921F6A78E4E3");
    }; // namespace Newton
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Newton::Mass::Class, Gek::Newton::Mass::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Newton::DynamicBody::Class, Gek::Newton::DynamicBody::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Newton::Player::Class, Gek::Newton::Player::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Newton::Class, Gek::Newton::System)
        ADD_CLASS_TYPE(Gek::Engine::System::Type)
END_CONTEXT_SOURCE