#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Components
    {
        // {06FD2921-89A3-490C-BBF6-9C909D921E96}
        DEFINE_GUID(MassClass, 0x6fd2921, 0x89a3, 0x490c, 0xbb, 0xf6, 0x9c, 0x90, 0x9d, 0x92, 0x1e, 0x96);

        // {5C52FBF3-261D-4016-9881-80E418C8E639}
        DEFINE_GUID(DynamicBodyClass, 0x5c52fbf3, 0x261d, 0x4016, 0x98, 0x81, 0x80, 0xe4, 0x18, 0xc8, 0xe6, 0x39);

        // {EF85AE66-9544-4B7C-A2CF-B2410BDA63BD}
        DEFINE_GUID(PlayerClass, 0xef85ae66, 0x9544, 0x4b7c, 0xa2, 0xcf, 0xb2, 0x41, 0xb, 0xda, 0x63, 0xbd);

        namespace Mass
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Mass

        namespace DynamicBody
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace DynamicBody

        namespace Player
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Player
    }; // namespace Components
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Components::MassClass, Gek::Components::Mass::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::DynamicBodyClass, Gek::Components::DynamicBody::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::PlayerClass, Gek::Components::Player::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)
END_CONTEXT_SOURCE