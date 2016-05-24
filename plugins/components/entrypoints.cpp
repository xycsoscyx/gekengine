#include <initguid.h>
#include <cguid.h>

#include "GEK\Context\COM.h"
#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

namespace Gek
{
    DECLARE_REGISTERED_CLASS(FirstPersonCameraImplementation);
    DECLARE_INTERFACE_IID(FirstPersonCameraRegistration, "4C33007B-108F-4C05-8A36-024888884AB7");

    DECLARE_REGISTERED_CLASS(ThirdPersonCameraImplementation);
    DECLARE_INTERFACE_IID(ThirdPersonCameraRegistration, "99AC75D7-6BF9-4065-B044-C16627C65349");

    DECLARE_REGISTERED_CLASS(CameraProcessorImplementation);
    DECLARE_INTERFACE_IID(CameraProcessorRegistration, "3EEB7CC0-94FB-4630-82DB-00281B00A28E");

    DECLARE_REGISTERED_CLASS(ColorImplementation);
    DECLARE_INTERFACE_IID(ColorRegistration, "A46EA40F-6325-4B4A-A20E-DD8B904D9EC3");

    DECLARE_REGISTERED_CLASS(PointLightImplementation);
    DECLARE_INTERFACE_IID(PointLightRegistration, "E4D8A791-319B-4324-8DBE-F99A7AAA6DB9");

    DECLARE_REGISTERED_CLASS(SpotLightImplementation);
    DECLARE_INTERFACE_IID(SpotLightRegistration, "71B0EACB-DA68-433E-B434-40DFC5206596");

    DECLARE_REGISTERED_CLASS(DirectionalLightImplementation);
    DECLARE_INTERFACE_IID(DirectionalLightRegistration, "DC611D73-15C6-4DED-9700-126E85D8B5F7");

    DECLARE_REGISTERED_CLASS(TransformImplementation);
    DECLARE_INTERFACE_IID(TransformRegistration, "62BAFD17-E5CB-4CA0-84B2-963836AE9836");
}; // namespace Gek

GEK_PLUGIN_BEGIN(Components)
    GEK_PLUGIN_CLASS(Gek::FirstPersonCameraRegistration, Gek::FirstPersonCameraImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::ThirdPersonCameraRegistration, Gek::ThirdPersonCameraImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::CameraProcessorRegistration, Gek::CameraProcessorImplementation)
        GEK_PLUGIN_TYPE(Gek::ProcessorType)

    GEK_PLUGIN_CLASS(Gek::ColorRegistration, Gek::ColorImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::PointLightRegistration, Gek::PointLightImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::SpotLightRegistration, Gek::SpotLightImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::DirectionalLightRegistration, Gek::DirectionalLightImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)

    GEK_PLUGIN_CLASS(Gek::TransformRegistration, Gek::TransformImplementation)
        GEK_PLUGIN_TYPE(Gek::ComponentType)
GEK_PLUGIN_END
