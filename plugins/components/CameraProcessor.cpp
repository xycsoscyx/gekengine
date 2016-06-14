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
    struct FirstPersonCameraComponent
    {
        float fieldOfView;
        float minimumDistance;
        float maximumDistance;

        FirstPersonCameraComponent(void)
            : fieldOfView(Math::convertDegreesToRadians(90.0f))
            , minimumDistance(1.0f)
            , maximumDistance(100.0f)
        {
        }

        void save(Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"field_of_view", fieldOfView);
            saveParameter(componentData, L"minimum_distance", minimumDistance);
            saveParameter(componentData, L"maximum_distance", maximumDistance);
        }

        void load(const Population::ComponentDefinition &componentData)
        {
            loadParameter(componentData, L"field_of_view", fieldOfView);
            loadParameter(componentData, L"minimum_distance", minimumDistance);
            loadParameter(componentData, L"maximum_distance", maximumDistance);
        }
    };

    class FirstPersonCameraImplementation
        : public ContextRegistration<FirstPersonCameraImplementation>
        , public ComponentMixin<FirstPersonCameraComponent>
    {
    public:
        FirstPersonCameraImplementation(Context *context)
            : ContextRegistration(context)
        {
        }

        // Component
        const wchar_t * const getName(void) const
        {
            return L"first_person_camera";
        }
    };

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

            population->listEntities<TransformComponent, FirstPersonCameraComponent>([&](Entity *entity) -> void
            {
                auto &data = entity->getComponent<FirstPersonCameraComponent>();

                float displayAspectRatio = (1280.0f / 800.0f);
                float fieldOfView = Math::convertDegreesToRadians(data.fieldOfView);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(fieldOfView, displayAspectRatio, data.minimumDistance, data.maximumDistance);

                render->render(entity, projectionMatrix, data.minimumDistance, data.maximumDistance);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCameraImplementation);
    GEK_REGISTER_CONTEXT_USER(CameraProcessorImplementation);
}; // namespace Gek
