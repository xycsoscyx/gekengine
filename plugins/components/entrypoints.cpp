#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(FirstPersonCameraImplementation);
    GEK_DECLARE_CONTEXT_USER(ThirdPersonCameraImplementation);
    GEK_DECLARE_CONTEXT_USER(CameraProcessorImplementation);
    GEK_DECLARE_CONTEXT_USER(ColorImplementation);
    GEK_DECLARE_CONTEXT_USER(PointLightImplementation);
    GEK_DECLARE_CONTEXT_USER(SpotLightImplementation);
    GEK_DECLARE_CONTEXT_USER(DirectionalLightImplementation);
    GEK_DECLARE_CONTEXT_USER(TransformImplementation);

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(FirstPersonCamera, FirstPersonCameraImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(ThirdPersonCamera, ThirdPersonCameraImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(CameraProcessor, CameraProcessorImplementation);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
        GEK_CONTEXT_ADD_CLASS(Color, ColorImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(PointLight, PointLightImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(SpotLight, SpotLightImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(DirectionalLight, DirectionalLightImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Transform, TransformImplementation);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
    GEK_CONTEXT_END();
}; // namespace Gek
