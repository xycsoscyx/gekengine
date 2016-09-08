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
        , public Plugin::PopulationStep
        , public Plugin::Processor
    {
    public:
        struct Camera
        {
            ResourceHandle target;
        };

    private:
        Plugin::Population *population;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        using EntityDataMap = std::unordered_map<Plugin::Entity *, Camera>;
        EntityDataMap entityDataMap;

    public:
        CameraProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

            population->addListener(this);
            population->addStep(this, 90);
        }

        ~CameraProcessor(void)
        {
            population->removeStep(this);
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
				const auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();

                Camera data;
                if (!cameraComponent.name.empty())
                {
                    auto backBuffer = renderer->getDevice()->getBackBuffer();
                    data.target = resources->createTexture(String::create(L"camera:%v", cameraComponent.name), Video::Format::R8G8B8A8_UNORM_SRGB, backBuffer->getWidth(), backBuffer->getHeight(), 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
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

        // Plugin::PopulationStep
        void onUpdate(uint32_t order, State state)
        {
            GEK_REQUIRE(renderer);

            if (state != State::Loading)
            {
                std::for_each(entityDataMap.begin(), entityDataMap.end(), [&](auto &entityDataPair) -> void
                {
					const Plugin::Entity *entity = entityDataPair.first;
					const auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();
					const auto &camera = entityDataPair.second;

					const auto backBuffer = renderer->getDevice()->getBackBuffer();
					const float width = float(backBuffer->getWidth());
					const float height = float(backBuffer->getHeight());

                    Math::Float4x4 projectionMatrix(Math::Float4x4::createPerspective(cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip));

                    renderer->render(entity, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, camera.target);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
