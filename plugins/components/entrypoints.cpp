#include <initguid.h>
#include <cguid.h>

#include "GEK\Engine\Component.h"
#include "GEK\Engine\Processor.h"

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
