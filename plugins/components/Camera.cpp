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

    class CameraImplementation : public ContextUserMixin
        , public ComponentMixin<CameraComponent>
    {
    public:
        CameraImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(CameraImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"camera";
        }
    };

    REGISTER_CLASS(CameraImplementation)
}; // namespace Gek