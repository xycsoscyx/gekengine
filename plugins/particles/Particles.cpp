#include "GEK\Engine\Particles.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ParticlesComponent::ParticlesComponent(void)
        : density(200)
        , size(1.0f)
    {
    }

    void ParticlesComponent::save(Population::ComponentDefinition &componentData) const
    {
        saveParameter(componentData, nullptr, material);
        saveParameter(componentData, L"density", density);
        saveParameter(componentData, L"color_map", colorMap);
        saveParameter(componentData, L"life_expectancy", lifeExpectancy);
        saveParameter(componentData, L"size", size);
    }

    void ParticlesComponent::load(const Population::ComponentDefinition &componentData)
    {
        loadParameter(componentData, nullptr, material);
        loadParameter(componentData, L"density", density);
        loadParameter(componentData, L"color_map", colorMap);
        loadParameter(componentData, L"life_expectancy", lifeExpectancy);
        loadParameter(componentData, L"size", size);
    }

    class ParticlesImplementation
        : public ContextRegistration<ParticlesImplementation>
        , public ComponentMixin<ParticlesComponent>
    {
    public:
        ParticlesImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"particles";
        }
    };

    GEK_REGISTER_CONTEXT_USER(ParticlesImplementation)
}; // namespace Gek