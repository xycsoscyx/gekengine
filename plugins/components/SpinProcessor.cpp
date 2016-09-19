#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Renderer.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <unordered_map>
#include <map>
#include <random>

namespace Gek
{
    namespace Components
    {
        static std::random_device randomDevice;
        static std::mt19937 mersineTwister(randomDevice());
        static std::uniform_real_distribution<float> random(-1.0f, 1.0f);

        struct Spin
        {
            Math::Float3 torque;

            Spin(void)
                : torque(random(mersineTwister), random(mersineTwister), random(mersineTwister))
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
            }
        };
    }; // namespace Components

    GEK_CONTEXT_USER(Spin)
        , public Plugin::ComponentMixin<Components::Spin>
    {
    public:
        Spin(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spin";
        }
    };

    GEK_CONTEXT_USER(SpinProcessor, Plugin::Core *)
        , public Plugin::PopulationListener
        , public Plugin::PopulationStep
        , public Plugin::Processor
    {
    public:

    private:
        Plugin::Population *population;

    public:
        SpinProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
        {
            GEK_REQUIRE(population);

            population->addListener(this);
            population->addStep(this, 0);
        }

        ~SpinProcessor(void)
        {
            population->removeStep(this);
            population->removeListener(this);
        }

        // Plugin::PopulationListener
        void onLoadBegin(void)
        {
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
        }

        void onEntityCreated(Plugin::Entity *entity)
        {
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
        }

        // Plugin::PopulationStep
        void onUpdate(uint32_t order, State state)
        {
            GEK_REQUIRE(population);

            if (state == State::Active)
            {
                population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity *entity) -> void
                {
                    auto &transform = entity->getComponent<Components::Transform>();
                    auto &spin = entity->getComponent<Components::Spin>();
                    Math::Quaternion rotation(Math::Quaternion::createEulerRotation((population->getFrameTime() * spin.torque.x),
                                                                                    (population->getFrameTime() * spin.torque.y),
                                                                                    (population->getFrameTime() * spin.torque.z)));
                    transform.rotation *= rotation;
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
