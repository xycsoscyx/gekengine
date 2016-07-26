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
#include <map>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        struct Spin
        {
            Spin(void)
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
        , public Plugin::Processor
    {
    public:

    private:
        Plugin::Population *population;
        uint32_t updateHandle;

    public:
        SpinProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , updateHandle(0)
        {
            GEK_REQUIRE(population);

            population->addListener(this);
            updateHandle = population->setUpdatePriority(this, 0);
        }

        ~SpinProcessor(void)
        {
            population->removeUpdatePriority(updateHandle);
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

        void onUpdate(uint32_t handle, State state)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER_TYPE(state, uint8_t));
            GEK_REQUIRE(population);

            if (state == State::Active)
            {
                Math::Quaternion rotation(0.0f, population->getFrameTime(), 0.0f);
                population->listEntities<Components::Transform, Components::Spin>([&](Plugin::Entity *entity) -> void
                {
                    auto &transform = entity->getComponent<Components::Transform>();
                    transform.rotation *= rotation;
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(Spin);
    GEK_REGISTER_CONTEXT_USER(SpinProcessor);
}; // namespace Gek
