#include "GEK\Components\Camera.h"
#include "GEK\Context\Plugin.h"
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

    HRESULT CameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"field_of_view", fieldOfView);
        saveParameter(componentData, L"minimum_distance", minimumDistance);
        saveParameter(componentData, L"maximum_distance", maximumDistance);
        return S_OK;
    }

    HRESULT CameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, L"field_of_view", fieldOfView);
        loadParameter(componentData, L"minimum_distance", minimumDistance);
        loadParameter(componentData, L"maximum_distance", maximumDistance);
        return S_OK;
    }

    FirstPersonCameraComponent::FirstPersonCameraComponent(void)
        : CameraComponent()
    {
    }

    HRESULT FirstPersonCameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        return CameraComponent::save(componentData);
    }

    HRESULT FirstPersonCameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        return CameraComponent::load(componentData);
    }

    ThirdPersonCameraComponent::ThirdPersonCameraComponent(void)
        : CameraComponent()
        , offset(0.0f)
        , distance(0.0f)
    {
    }

    HRESULT ThirdPersonCameraComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, L"body", body);
        saveParameter(componentData, L"offset", offset);
        saveParameter(componentData, L"distance", distance);
        return CameraComponent::save(componentData);
    }

    HRESULT ThirdPersonCameraComponent::load(const Population::ComponentDefinition &componentData)
    {
        if (!loadParameter(componentData, L"body", body))
        {
            return E_INVALIDARG;
        }

        loadParameter(componentData, L"offset", offset);
        loadParameter(componentData, L"distance", distance);
        return CameraComponent::load(componentData);
    }

    class FirstPersonCameraImplementation
        : public ContextUserMixin
        , public ComponentMixin<FirstPersonCameraComponent>
    {
    public:
        FirstPersonCameraImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(FirstPersonCameraImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"first_person_camera";
        }
    };

    class ThirdPersonCameraImplementation
        : public ContextUserMixin
        , public ComponentMixin<ThirdPersonCameraComponent>
    {
    public:
        ThirdPersonCameraImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ThirdPersonCameraImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"third_person_camera";
        }
    };

    REGISTER_CLASS(FirstPersonCameraImplementation)
    REGISTER_CLASS(ThirdPersonCameraImplementation)
}; // namespace Gek