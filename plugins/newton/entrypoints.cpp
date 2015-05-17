#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"
#include "GEK\Engine\ComponentSystemInterface.h"

namespace Gek
{
    namespace Newton
    {
        namespace Components
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
        }; // namespace Components

        namespace System
        {
            DECLARE_REGISTERED_CLASS(System);
            DECLARE_INTERFACE_IID(Class, "BCF7CC29-F375-4F29-9DA7-921F6A78E4E3");
        }; // namespace System
    }; // namespace Newton
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Newton::Components::Mass::Class, Gek::Newton::Components::Mass::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Newton::Components::DynamicBody::Class, Gek::Newton::Components::DynamicBody::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Newton::Components::Player::Class, Gek::Newton::Components::Player::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Newton::System::Class, Gek::Newton::System::System)
        ADD_CLASS_TYPE(Gek::ComponentSystemType)
END_CONTEXT_SOURCE