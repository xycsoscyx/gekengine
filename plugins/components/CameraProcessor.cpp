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
        float minimumDistance;
        float maximumDistance;
        uint32_t width;
        uint32_t height;
        String format;

        FirstPersonCameraComponent(void)
        {
        }

        void save(Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, L"field_of_view", Math::convertRadiansToDegrees(fieldOfView));
            saveParameter(componentData, L"minimum_distance", minimumDistance);
            saveParameter(componentData, L"maximum_distance", maximumDistance);
            saveParameter(componentData, L"width", width);
            saveParameter(componentData, L"height", height);
            saveParameter(componentData, L"format", format);
        }

        void load(const Population::ComponentDefinition &componentData)
        {
            fieldOfView = Math::convertDegreesToRadians(loadParameter(componentData, L"field_of_view", 90.0f));
            minimumDistance = loadParameter(componentData, L"minimum_distance", 1.0f);
            maximumDistance = loadParameter(componentData, L"maximum_distance", 100.0f);
            width = loadParameter(componentData, L"width", 0);
            height = loadParameter(componentData, L"height", 0);
            format = loadParameter(componentData, L"format", String());
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
        struct Data
        {
            ResourceHandle target;
        };

    private:
        Population *population;
        uint32_t updateHandle;
        Resources *resources;
        Render *render;

        typedef std::unordered_map<Entity *, Data> EntityDataMap;
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

                Data entityData;
                if (!cameraComponent.format.empty() &&
                    cameraComponent.width > 0 &&
                    cameraComponent.height > 0)
                {
                    String name;
                    if (entity->getName())
                    {
                        name.format(L"camera:%v", entity->getName());
                    }
                    else
                    {
                        name.format(L"camera:%v", entity);
                    }

                    entityData.target = resources->createTexture(name, Video::Format::sRGBA, cameraComponent.width, cameraComponent.height, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                }

                entityDataMap.insert(std::make_pair(entity, entityData));
            }
        }

        void onEntityDestroyed(Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entityDataIterator = entityDataMap.find(entity);
            if (entityDataIterator != entityDataMap.end())
            {
                entityDataMap.erase(entityDataIterator);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(render);

            concurrency::parallel_for_each(entityDataMap.begin(), entityDataMap.end(), [&](EntityDataMap::value_type &entityData) -> void
            {
                Entity *entity = entityData.first;
                auto &camera = entity->getComponent<FirstPersonCameraComponent>();
                auto &data = entityData.second;

                float displayAspectRatio;
                if (data.target)
                {
                    displayAspectRatio = (float(camera.width) / float(camera.height));
                }
                else
                {
                    float width = float(render->getVideoSystem()->getBackBuffer()->getWidth());
                    float height = float(render->getVideoSystem()->getBackBuffer()->getHeight());
                    displayAspectRatio = (width / height);
                }

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(camera.fieldOfView, displayAspectRatio, camera.minimumDistance, camera.maximumDistance);

                render->render(entity, projectionMatrix, camera.minimumDistance, camera.maximumDistance, data.target);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCameraImplementation);
    GEK_REGISTER_CONTEXT_USER(CameraProcessorImplementation);
}; // namespace Gek
