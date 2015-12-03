#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(CameraImplementation);
    DECLARE_INTERFACE_IID(CameraRegistration, "4C33007B-108F-4C05-8A36-024888884AB7");

    DECLARE_REGISTERED_CLASS(ColorImplementation);
    DECLARE_INTERFACE_IID(ColorRegistration, "A46EA40F-6325-4B4A-A20E-DD8B904D9EC3");

    DECLARE_REGISTERED_CLASS(FollowImplementation);
    DECLARE_INTERFACE_IID(FollowRegistration, "E9B44929-02E1-4093-B22B-414D1E5DA1FD");

    DECLARE_REGISTERED_CLASS(PointLightImplementation);
    DECLARE_INTERFACE_IID(PointLightRegistration, "E4D8A791-319B-4324-8DBE-F99A7AAA6DB9");

    DECLARE_REGISTERED_CLASS(SizeImplementation);
    DECLARE_INTERFACE_IID(SizeRegistration, "EA3E463D-2A25-4886-B4A2-C035C641D4B8");

    DECLARE_REGISTERED_CLASS(TransformImplementation);
    DECLARE_INTERFACE_IID(TransformRegistration, "62BAFD17-E5CB-4CA0-84B2-963836AE9836");
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::CameraRegistration, Gek::CameraImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::ColorRegistration, Gek::ColorImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::PointLightRegistration, Gek::PointLightImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::SizeRegistration, Gek::SizeImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::TransformRegistration, Gek::TransformImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)
END_CONTEXT_SOURCE