#include "GEK\Context\ContextUser.h"

GEK_CONTEXT_BEGIN(Components)
    GEK_CONTEXT_ADD_CLASS(Gek::FirstPersonCameraRegistration, Gek::FirstPersonCameraImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::ThirdPersonCameraRegistration, Gek::ThirdPersonCameraImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::CameraProcessorRegistration, Gek::CameraProcessorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ProcessorType)

    GEK_CONTEXT_ADD_CLASS(Gek::ColorRegistration, Gek::ColorImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::PointLightRegistration, Gek::PointLightImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::SpotLightRegistration, Gek::SpotLightImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::DirectionalLightRegistration, Gek::DirectionalLightImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)

    GEK_CONTEXT_ADD_CLASS(Gek::TransformRegistration, Gek::TransformImplementation)
        GEK_CONTEXT_ADD_TYPE(Gek::ComponentType)
GEK_CONTEXT_END()
