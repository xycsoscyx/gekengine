#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Camera
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "4C33007B-108F-4C05-8A36-024888884AB7");
            }; // namespace Camera

            namespace Color
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "A46EA40F-6325-4B4A-A20E-DD8B904D9EC3");
            }; // namespace Color

            namespace Follow
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "E9B44929-02E1-4093-B22B-414D1E5DA1FD");
            }; // namespace Follow

            namespace PointLight
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "E4D8A791-319B-4324-8DBE-F99A7AAA6DB9");
            }; // namespace PointLight

            namespace Size
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "EA3E463D-2A25-4886-B4A2-C035C641D4B8");
            }; // namespace Size

            namespace Transform
            {
                DECLARE_REGISTERED_CLASS(Component);
                DECLARE_INTERFACE_IID(Class, "62BAFD17-E5CB-4CA0-84B2-963836AE9836");
            }; // namespace Transform
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Engine::Components::Camera::Class, Gek::Engine::Components::Camera::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Engine::Components::Color::Class, Gek::Engine::Components::Color::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Engine::Components::Follow::Class, Gek::Engine::Components::Follow::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Engine::Components::PointLight::Class, Gek::Engine::Components::PointLight::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Engine::Components::Size::Class, Gek::Engine::Components::Size::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)

    ADD_CONTEXT_CLASS(Gek::Engine::Components::Transform::Class, Gek::Engine::Components::Transform::Component)
        ADD_CLASS_TYPE(Gek::Engine::Component::Type)
END_CONTEXT_SOURCE