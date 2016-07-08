#include "GEK\Context\ContextUser.h"

namespace Gek
{
    GEK_DECLARE_CONTEXT_USER(FirstPersonCamera);
    GEK_DECLARE_CONTEXT_USER(CameraProcessor);
    GEK_DECLARE_CONTEXT_USER(Filter);
    GEK_DECLARE_CONTEXT_USER(Color);
    GEK_DECLARE_CONTEXT_USER(PointLight);
    GEK_DECLARE_CONTEXT_USER(SpotLight);
    GEK_DECLARE_CONTEXT_USER(DirectionalLight);
    GEK_DECLARE_CONTEXT_USER(Transform);
    GEK_DECLARE_CONTEXT_USER(Spin);
    GEK_DECLARE_CONTEXT_USER(SpinProcessor);

    GEK_CONTEXT_BEGIN(System);
        GEK_CONTEXT_ADD_CLASS(Components::FirstPersonCamera, FirstPersonCamera);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::Filter, Filter);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::Color, Color);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::PointLight, PointLight);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::SpotLight, SpotLight);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::DirectionalLight, DirectionalLight);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::Transform, Transform);
            GEK_CONTEXT_ADD_TYPE(ComponentType);
        GEK_CONTEXT_ADD_CLASS(Components::Spin, Spin);
            GEK_CONTEXT_ADD_TYPE(ComponentType);

        GEK_CONTEXT_ADD_CLASS(Processors::CameraProcessor, CameraProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
        GEK_CONTEXT_ADD_CLASS(Processors::SpinProcessor, SpinProcessor);
            GEK_CONTEXT_ADD_TYPE(ProcessorType);
    GEK_CONTEXT_END();
}; // namespace Gek
