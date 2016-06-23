#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <map>
#include <unordered_map>

namespace Gek
{
    struct SpinComponent
    {
        SpinComponent(void)
        {
        }

        void save(Population::ComponentDefinition &componentData) const
        {
        }

        void load(const Population::ComponentDefinition &componentData)
        {
        }
    };

    class SpinImplementation
        : public ContextRegistration<SpinImplementation>
        , public ComponentMixin<SpinComponent>
    {
    public:
        SpinImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"spin";
        }
    };

    class SpinProcessorImplementation
        : public ContextRegistration<SpinProcessorImplementation, EngineContext *>
        , public PopulationObserver
        , public Processor
    {
    public:

    private:
        Population *population;
        uint32_t updateHandle;

    public:
        SpinProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , population(engine->getPopulation())
            , updateHandle(0)
        {
            population->addObserver((PopulationObserver *)this);
            updateHandle = population->setUpdatePriority(this, 0);
        }

        ~SpinProcessorImplementation(void)
        {
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeObserver((PopulationObserver *)this);
            }
        }

        // PopulationObserver
        void onLoadBegin(void)
        {
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
            onFree();
        }

        void onFree(void)
        {
        }

        void onEntityCreated(Entity *head)
        {
            GEK_REQUIRE(population);
        }

        void onEntityDestroyed(Entity *head)
        {
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));
            GEK_REQUIRE(population);

            Math::Quaternion rotation(0.0f, population->getFrameTime(), 0.0f);
            population->listEntities<TransformComponent, SpinComponent>([&](Entity *entity) -> void
            {
                auto &transform = entity->getComponent<TransformComponent>();
                transform.rotation *= rotation;
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(SpinImplementation);
    GEK_REGISTER_CONTEXT_USER(SpinProcessorImplementation);
}; // namespace Gek
