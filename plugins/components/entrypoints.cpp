#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\Common.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(CameraImplementation);
    DECLARE_INTERFACE_IID(CameraRegistration, "4C33007B-108F-4C05-8A36-024888884AB7");

    DECLARE_REGISTERED_CLASS(ColorImplementation);
    DECLARE_INTERFACE_IID(ColorRegistration, "A46EA40F-6325-4B4A-A20E-DD8B904D9EC3");

    DECLARE_REGISTERED_CLASS(PointLightImplementation);
    DECLARE_INTERFACE_IID(PointLightRegistration, "E4D8A791-319B-4324-8DBE-F99A7AAA6DB9");

    DECLARE_REGISTERED_CLASS(ScaleImplementation);
    DECLARE_INTERFACE_IID(ScaleRegistration, "EA3E463D-2A25-4886-B4A2-C035C641D4B8");

    DECLARE_REGISTERED_CLASS(TransformImplementation);
    DECLARE_INTERFACE_IID(TransformRegistration, "62BAFD17-E5CB-4CA0-84B2-963836AE9836");

    DECLARE_REGISTERED_CLASS(FollowImplementation);
    DECLARE_INTERFACE_IID(FollowRegistration, "99AC75D7-6BF9-4065-B044-C16627C65349");

    DECLARE_REGISTERED_CLASS(FollowProcessorImplementation);
    DECLARE_INTERFACE_IID(FollowProcessorRegistration, "3EEB7CC0-94FB-4630-82DB-00281B00A28E");
}; // namespace Gek

DECLARE_CONTEXT_SOURCE(Components)
    ADD_CONTEXT_CLASS(Gek::CameraRegistration, Gek::CameraImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::ColorRegistration, Gek::ColorImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::PointLightRegistration, Gek::PointLightImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::ScaleRegistration, Gek::ScaleImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::TransformRegistration, Gek::TransformImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::FollowRegistration, Gek::FollowImplementation)
        ADD_CLASS_TYPE(Gek::ComponentType)

    ADD_CONTEXT_CLASS(Gek::FollowProcessorRegistration, Gek::FollowProcessorImplementation)
        ADD_CLASS_TYPE(Gek::ProcessorType)
END_CONTEXT_SOURCE