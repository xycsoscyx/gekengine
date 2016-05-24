#include "GEK\Engine\Particles.h"
#include "GEK\Context\Plugin.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ParticlesComponent::ParticlesComponent(void)
        : density(200)
        , size(1.0f)
    {
    }

    HRESULT ParticlesComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, material);
        saveParameter(componentData, L"density", density);
        saveParameter(componentData, L"color_map", colorMap);
        saveParameter(componentData, L"life_expectancy", lifeExpectancy);
        saveParameter(componentData, L"size", size);
        return S_OK;
    }

    HRESULT ParticlesComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, material);
        loadParameter(componentData, L"density", density);
        loadParameter(componentData, L"color_map", colorMap);
        loadParameter(componentData, L"life_expectancy", lifeExpectancy);
        loadParameter(componentData, L"size", size);
        return S_OK;
    }

    class ParticlesImplementation
        : public ContextUserMixin
        , public ComponentMixin<ParticlesComponent>
    {
    public:
        ParticlesImplementation(void)
        {
        }

        BEGIN_INTERFACE_LIST(ParticlesImplementation)
            INTERFACE_LIST_ENTRY_COM(Component)
        END_INTERFACE_LIST_USER

        // Component
        STDMETHODIMP_(LPCWSTR) getName(void) const
        {
            return L"particles";
        }
    };

    REGISTER_CLASS(ParticlesImplementation)
}; // namespace Gek