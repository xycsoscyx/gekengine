#include "GEK\Components\Camera.h"
#include "GEK\Context\ContextUserMixin.h"
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

    HRESULT CameraComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        saveParameter(componentParameterList, L"field_of_view", fieldOfView);
        saveParameter(componentParameterList, L"minimum_distance", minimumDistance);
        saveParameter(componentParameterList, L"maximum_distance", maximumDistance);
        return S_OK;
    }

    HRESULT CameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        loadParameter(componentParameterList, L"field_of_view", fieldOfView);
        loadParameter(componentParameterList, L"minimum_distance", minimumDistance);
        loadParameter(componentParameterList, L"maximum_distance", maximumDistance);
        return S_OK;
    }

    FirstPersonCameraComponent::FirstPersonCameraComponent(void)
        : CameraComponent()
    {
    }

    HRESULT FirstPersonCameraComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        return CameraComponent::save(componentParameterList);
    }

    HRESULT FirstPersonCameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        return CameraComponent::load(componentParameterList);
    }

    ThirdPersonCameraComponent::ThirdPersonCameraComponent(void)
        : CameraComponent()
        , offset(0.0f)
        , distance(0.0f)
    {
    }

    HRESULT ThirdPersonCameraComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        saveParameter(componentParameterList, L"body", body);
        saveParameter(componentParameterList, L"offset", offset);
        saveParameter(componentParameterList, L"distance", distance);
        return CameraComponent::save(componentParameterList);
    }

    HRESULT ThirdPersonCameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        if (!loadParameter(componentParameterList, L"body", body))
        {
            return E_INVALIDARG;
        }

        loadParameter(componentParameterList, L"offset", offset);
        loadParameter(componentParameterList, L"distance", distance);
        return CameraComponent::load(componentParameterList);
    }

    class FirstPersonCameraImplementation : public ContextUserMixin
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

    class ThirdPersonCameraImplementation : public ContextUserMixin
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