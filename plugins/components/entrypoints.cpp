#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\ComponentInterface.h"

namespace Gek
{
    namespace Components
    {
        // {4C33007B-108F-4C05-8A36-024888884AB7}
        DEFINE_GUID(CameraClass, 0x4c33007b, 0x108f, 0x4c05, 0x8a, 0x36, 0x2, 0x48, 0x88, 0x88, 0x4a, 0xb7);

        // {A46EA40F-6325-4B4A-A20E-DD8B904D9EC3}
        DEFINE_GUID(ColorClass, 0xa46ea40f, 0x6325, 0x4b4a, 0xa2, 0xe, 0xdd, 0x8b, 0x90, 0x4d, 0x9e, 0xc3);

        // {E9B44929-02E1-4093-B22B-414D1E5DA1FD}
        DEFINE_GUID(FollowClass, 0xe9b44929, 0x2e1, 0x4093, 0xb2, 0x2b, 0x41, 0x4d, 0x1e, 0x5d, 0xa1, 0xfd);

        // {E4D8A791-319B-4324-8DBE-F99A7AAA6DB9}
        DEFINE_GUID(PointLightClass, 0xe4d8a791, 0x319b, 0x4324, 0x8d, 0xbe, 0xf9, 0x9a, 0x7a, 0xaa, 0x6d, 0xb9);

        // {EA3E463D-2A25-4886-B4A2-C035C641D4B8}
        DEFINE_GUID(SizeClass, 0xea3e463d, 0x2a25, 0x4886, 0xb4, 0xa2, 0xc0, 0x35, 0xc6, 0x41, 0xd4, 0xb8);

        // {62BAFD17-E5CB-4CA0-84B2-963836AE9836}
        DEFINE_GUID(TransformClass, 0x62bafd17, 0xe5cb, 0x4ca0, 0x84, 0xb2, 0x96, 0x38, 0x36, 0xae, 0x98, 0x36);

        namespace Camera
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Camera

        namespace Color
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Color

        namespace Follow
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Follow

        namespace PointLight
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace PointLight

        namespace Size
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Size

        namespace Transform
        {
            DECLARE_REGISTERED_CLASS(Component);
        }; // namespace Transform
    }; // namespace Components
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::Components::CameraClass, Gek::Components::Camera::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::ColorClass, Gek::Components::Color::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::FollowClass, Gek::Components::Follow::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::PointLightClass, Gek::Components::PointLight::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::SizeClass, Gek::Components::Size::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::Components::TransformClass, Gek::Components::Transform::Component)
        ADD_CLASS_TYPE(Gek::ComponentType)
END_CONTEXT_SOURCE