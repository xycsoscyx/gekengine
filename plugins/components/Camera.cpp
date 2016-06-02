#include "GEK\Components\Camera.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    CameraComponent::CameraComponent(void)
        : fieldOfView(Math::convertDegreesToRadians(90.0f))
        , minimumDistance(1.0f)
        , maximumDistance(100.0f)
    {
    }

    void CameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"field_of_view", fieldOfView);
        saveParameter(componentData, L"minimum_distance", minimumDistance);
        saveParameter(componentData, L"maximum_distance", maximumDistance);
    }

    void CameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"field_of_view", fieldOfView);
        loadParameter(componentData, L"minimum_distance", minimumDistance);
        loadParameter(componentData, L"maximum_distance", maximumDistance);
    }

    FirstPersonCameraComponent::FirstPersonCameraComponent(void)
        : CameraComponent()
    {
    }

    void FirstPersonCameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        CameraComponent::save(componentData);
    }

    void FirstPersonCameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        CameraComponent::load(componentData);
    }

    ThirdPersonCameraComponent::ThirdPersonCameraComponent(void)
        : CameraComponent()
        , offset(0.0f)
        , distance(0.0f)
    {
    }

    void ThirdPersonCameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"body", body);
        saveParameter(componentData, L"offset", offset);
        saveParameter(componentData, L"distance", distance);
        CameraComponent::save(componentData);
    }

    void ThirdPersonCameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"body", body);
        loadParameter(componentData, L"offset", offset);
        loadParameter(componentData, L"distance", distance);
        CameraComponent::load(componentData);
    }

    class FirstPersonCameraImplementation
        : public ContextRegistration<FirstPersonCameraImplementation>
        , public ComponentMixin<FirstPersonCameraComponent>
    {
    public:
        FirstPersonCameraImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"first_person_camera";
        }
    };

    class ThirdPersonCameraImplementation
        : public ContextRegistration<ThirdPersonCameraImplementation>
        , public ComponentMixin<ThirdPersonCameraComponent>
    {
    public:
        ThirdPersonCameraImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"third_person_camera";
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCameraImplementation);
    GEK_REGISTER_CONTEXT_USER(ThirdPersonCameraImplementation);
}; // namespace Gek