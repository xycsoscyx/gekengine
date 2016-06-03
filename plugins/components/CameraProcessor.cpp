#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Engine.h"
#include "GEK\Engine\Processor.h"
#include "GEK\Engine\Population.h"
#include "GEK\Engine\Render.h"
#include "GEK\Engine\Entity.h"
#include "GEK\Components\Transform.h"
#include "GEK\Components\Camera.h"
#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include <map>
#include <unordered_map>

namespace Gek
{
    class CameraProcessorImplementation
        : public ContextRegistration<CameraProcessorImplementation, EngineContext *>
        , public PopulationObserver
        , public Processor
    {
    public:

    private:
        Population *population;
        uint32_t updateHandle;
        Render *render;

    public:
        CameraProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , population(engine->getPopulation())
            , updateHandle(0)
            , render(engine->getRender())
        {
            population->addObserver((PopulationObserver *)this);
            updateHandle = population->setUpdatePriority(this, 90);
        }

        ~CameraProcessorImplementation(void)
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
            GEK_REQUIRE(population);
            GEK_REQUIRE(render);

            population->listEntities<TransformComponent, FirstPersonCameraComponent>([&](Entity *cameraEntity) -> void
            {
                auto &cameraData = cameraEntity->getComponent<FirstPersonCameraComponent>();

                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(cameraData.fieldOfView);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, cameraData.minimumDistance, cameraData.maximumDistance);

                render->render(cameraEntity, projectionMatrix, cameraData.minimumDistance, cameraData.maximumDistance);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(CameraProcessorImplementation);
}; // namespace Gek
