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
        , public Plugin::PopulationListener
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
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->addListener(this);
            updateHandle = population->setUpdatePriority(this, 90);
        }

        ~CameraProcessor(void)
        {
            population->removeUpdatePriority(updateHandle);
            population->removeListener(this);
        }

        // Plugin::PopulationListener
        void onLoadBegin(void)
        {
            entityDataMap.clear();
        }

        void onLoadSucceeded(void)
        {
        }

        void onLoadFailed(void)
        {
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
                    data.target = resources->createTexture(name, Video::Format::R8G8B8A8_UNORM_SRGB, backBuffer->getWidth(), backBuffer->getHeight(), 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
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

        void onUpdate(uint32_t handle, State state)
        {
            GEK_TRACE_SCOPE(GEK_PARAMETER(handle), GEK_PARAMETER_TYPE(state, uint8_t));
            GEK_REQUIRE(renderer);

            if (state != State::Loading)
            {
                std::for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entityDataPair) -> void
                {
                    Plugin::Entity *entity = entityDataPair.first;
                    auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();
                    auto &camera = entityDataPair.second;

                    auto backBuffer = renderer->getDevice()->getBackBuffer();
                    float width = float(backBuffer->getWidth());
                    float height = float(backBuffer->getHeight());
                    float displayAspectRatio = (width / height);

                    Math::Float4x4 projectionMatrix;
                    projectionMatrix.setPerspective(cameraComponent.fieldOfView, displayAspectRatio, cameraComponent.nearClip, cameraComponent.farClip);

                    renderer->render(entity, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, camera.target);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
