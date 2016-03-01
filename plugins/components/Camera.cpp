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
        componentParameterList[L"field_of_view"] = String::from(fieldOfView);
        componentParameterList[L"minimum_distance"] = String::from(minimumDistance);
        componentParameterList[L"maximum_distance"] = String::from(maximumDistance);
        return S_OK;
    }

    HRESULT CameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"field_of_view", fieldOfView, String::to<float>);
        setParameter(componentParameterList, L"minimum_distance", minimumDistance, String::to<float>);
        setParameter(componentParameterList, L"maximum_distance", maximumDistance, String::to<float>);
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
        componentParameterList[L"body"] = body;
        componentParameterList[L"offset"] = String::from(offset);
        componentParameterList[L"distance"] = String::from(distance);
        return CameraComponent::save(componentParameterList);
    }

    HRESULT ThirdPersonCameraComponent::load(const std::unordered_map<CStringW, CStringW> &componentParameterList)
    {
        setParameter(componentParameterList, L"body", body, [](LPCWSTR value) -> LPCWSTR { return value; });
        setParameter(componentParameterList, L"offset", offset, String::to<Math::Float3>);
        setParameter(componentParameterList, L"distance", distance, String::to<Math::Float3>);
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