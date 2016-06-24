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
#include <unordered_map>
#include <map>
#include <ppl.h>

namespace Gek
{
    struct FirstPersonCameraComponent
    {
        float fieldOfView;
        float nearClip;
        float farClip;
        String name;

        FirstPersonCameraComponent(void)
        {
        }

        void save(Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"field_of_view", Math::convertRadiansToDegrees(fieldOfView));
            saveParameter(componentData, L"near_clip", nearClip);
            saveParameter(componentData, L"far_clip", farClip);
            saveParameter(componentData, L"name", name);
        }

        void load(const Population::ComponentDefinition &componentData)
        {
            fieldOfView = Math::convertDegreesToRadians(loadParameter(componentData, L"field_of_view", 90.0f));
            nearClip = loadParameter(componentData, L"near_clip", 1.0f);
            farClip = loadParameter(componentData, L"far_clip", 100.0f);
            name = loadParameter(componentData, L"name", String());
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
        struct Camera
        {
            ResourceHandle target;
        };

    private:
        Population *population;
        uint32_t updateHandle;
        Resources *resources;
        Render *render;

        typedef std::unordered_map<Entity *, Camera> EntityDataMap;
        EntityDataMap entityDataMap;

    public:
        CameraProcessorImplementation(Context *context, EngineContext *engine)
            : ContextRegistration(context)
            , population(engine->getPopulation())
            , updateHandle(0)
            , resources(engine->getResources())
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
            entityDataMap.clear();
        }

        void onEntityCreated(Entity *entity)
        {
            GEK_REQUIRE(entity);

            if (entity->hasComponents<FirstPersonCameraComponent, TransformComponent>())
            {
                auto &cameraComponent = entity->getComponent<FirstPersonCameraComponent>();

                Camera data;
                if (!cameraComponent.name.empty())
                {
                    String name(L"camera:%v", cameraComponent.name);
                    data.target = resources->createTexture(name, Video::Format::sRGBA, render->getVideoSystem()->getBackBuffer()->getWidth(), render->getVideoSystem()->getBackBuffer()->getHeight(), 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                }

                entityDataMap.insert(std::make_pair(entity, data));
            }
        }

        void onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto data = entityDataMap.find(entity);
            if (data != entityDataMap.end())
            {
                entityDataMap.erase(data);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));
            GEK_REQUIRE(render);

            std::for_each(entityDataMap.begin(), entityDataMap.end(), [&](EntityDataMap::value_type &data) -> void
            {
                Entity *entity = data.first;
                auto &cameraComponent = entity->getComponent<FirstPersonCameraComponent>();
                auto &camera = data.second;

                float width = float(render->getVideoSystem()->getBackBuffer()->getWidth());
                float height = float(render->getVideoSystem()->getBackBuffer()->getHeight());
                float displayAspectRatio = (width / height);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(cameraComponent.fieldOfView, displayAspectRatio, cameraComponent.nearClip, cameraComponent.farClip);

                render->render(entity, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, camera.target);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCameraImplementation);
    GEK_REGISTER_CONTEXT_USER(CameraProcessorImplementation);
}; // namespace Gek
