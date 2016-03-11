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
        getParameter(componentParameterList, L"field_of_view", fieldOfView);
        getParameter(componentParameterList, L"minimum_distance", minimumDistance);
        getParameter(componentParameterList, L"maximum_distance", maximumDistance);
        return S_OK;
    }

    HRESULT CameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"field_of_view", fieldOfView);
        setParameter(componentParameterList, L"minimum_distance", minimumDistance);
        setParameter(componentParameterList, L"maximum_distance", maximumDistance);
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
    {
    }

    HRESULT ThirdPersonCameraComponent::save(std::unordered_map<CStringW, CStringW> &componentParameterList) const
    {
        getParameter(componentParameterList, L"body", body);
        getParameter(componentParameterList, L"offset", offset);
        getParameter(componentParameterList, L"distance", distance);
        return CameraComponent::save(componentParameterList);
    }

    HRESULT ThirdPersonCameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"body", body);
        setParameter(componentParameterList, L"offset", offset);
        setParameter(componentParameterList, L"distance", distance);
        return CameraComponent::load(componentParameterList);
    }

    class FirstPersonCameraImplementation : public ContextUserMixin
        , public ComponentMixin<CameraComponent>
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
        , public ComponentMixin<CameraComponent>
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