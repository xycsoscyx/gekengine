#include "GEK\Engine\Particles.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    ParticlesComponent::ParticlesComponent(void)
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
        material = loadParameter<String>(componentData, nullptr);
        density = loadParameter(componentData, L"density", 100);
        colorMap = loadParameter<String>(componentData, L"color_map");
        lifeExpectancy = loadParameter(componentData, L"life_expectancy", Math::Float2(1.0f, 1.0f));
        size = loadParameter(componentData, L"size", Math::Float2(1.0f, 1.0f));
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