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
#include <ppl.h>

namespace Gek
{
    namespace Components
    {
        struct FirstPersonCamera
        {
            float fieldOfView;
            float nearClip;
            float farClip;
            String name;

            FirstPersonCamera(void)
            {
            }

            void save(Plugin::Population::ComponentDefinition &componentData) const
            {
                saveParameter(componentData, L"field_of_view", Math::convertRadiansToDegrees(fieldOfView));
                saveParameter(componentData, L"near_clip", nearClip);
                saveParameter(componentData, L"far_clip", farClip);
                saveParameter(componentData, L"name", name);
            }

            void load(const Plugin::Population::ComponentDefinition &componentData)
            {
                fieldOfView = Math::convertDegreesToRadians(loadParameter(componentData, L"field_of_view", 90.0f));
                nearClip = loadParameter(componentData, L"near_clip", 1.0f);
                farClip = loadParameter(componentData, L"far_clip", 100.0f);
                name = loadParameter(componentData, L"name", String());
            }
        };
    };

    GEK_CONTEXT_USER(FirstPersonCamera)
        , public Plugin::ComponentMixin<Components::FirstPersonCamera>
    {
    public:
        FirstPersonCamera(Context *context)
            : ContextRegistration(context)
        {
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"first_person_camera";
        }
    };

    GEK_CONTEXT_USER(CameraProcessor, Plugin::Core *)
        , public Plugin::PopulationObserver
        , public Plugin::Processor
    {
    public:
        struct Camera
        {
            ResourceHandle target;
        };

    private:
        Plugin::Population *population;
        uint32_t updateHandle;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        using EntityDataMap = std::unordered_map<Plugin::Entity *, Camera>;
        EntityDataMap entityDataMap;

    public:
        CameraProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , updateHandle(0)
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            population->addObserver((Plugin::PopulationObserver *)this);
            updateHandle = population->setUpdatePriority(this, 90);
        }

        ~CameraProcessor(void)
        {
            if (population)
            {
                population->removeUpdatePriority(updateHandle);
                population->removeObserver((Plugin::PopulationObserver *)this);
            }
        }

        // Plugin::PopulationObserver
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

        void onEntityCreated(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            if (entity->hasComponents<Components::FirstPersonCamera, Components::Transform>())
            {
                auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();

                Camera data;
                if (!cameraComponent.name.empty())
                {
                    String name(L"camera:%v", cameraComponent.name);
                    auto backBuffer = renderer->getDevice()->getBackBuffer();
                    data.target = resources->createTexture(name, Video::Format::sRGBA, backBuffer->getWidth(), backBuffer->getHeight(), 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource, false);
                }

                entityDataMap.insert(std::make_pair(entity, data));
            }
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            GEK_REQUIRE(entity);

            auto entitySearch = entityDataMap.find(entity);
            if (entitySearch != entityDataMap.end())
            {
                entityDataMap.erase(entitySearch);
            }
        }

        void onUpdate(uint32_t handle, bool isIdle)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER(isIdle));
            GEK_REQUIRE(renderer);

            std::for_each(entityDataMap.begin(), entityDataMap.end(), [&](EntityDataMap::value_type &data) -> void
            {
                Plugin::Entity *entity = data.first;
                auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();
                auto &camera = data.second;

                auto backBuffer = renderer->getDevice()->getBackBuffer();
                float width = float(backBuffer->getWidth());
                float height = float(backBuffer->getHeight());
                float displayAspectRatio = (width / height);

                Math::Float4x4 projectionMatrix;
                projectionMatrix.setPerspective(cameraComponent.fieldOfView, displayAspectRatio, cameraComponent.nearClip, cameraComponent.farClip);

                renderer->render(entity, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, camera.target);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
