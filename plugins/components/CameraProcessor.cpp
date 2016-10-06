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
#include <ppl.h>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(FirstPersonCamera)
        {
            float fieldOfView;
            float nearClip;
            float farClip;
            String name;

            void save(Xml::Leaf &componentData) const
            {
                componentData.attributes[L"field_of_view"] = Math::convertRadiansToDegrees(fieldOfView);
                componentData.attributes[L"near_clip"] = nearClip;
                componentData.attributes[L"far_clip"] = farClip;
                componentData.attributes[L"name"] = name;
            }

            void load(const Xml::Leaf &componentData)
            {
                fieldOfView = Math::convertDegreesToRadians(componentData.getValue(L"field_of_view", 90.0f));
                nearClip = componentData.getValue(L"near_clip", 1.0f);
                farClip = componentData.getValue(L"far_clip", 100.0f);
                name = componentData.getAttribute(L"name");
            }
        };
    };

    GEK_CONTEXT_USER(FirstPersonCamera)
        , public Plugin::ComponentMixin<Components::FirstPersonCamera, Editor::Component>
    {
    public:
        FirstPersonCamera(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &firstPersonCameraComponent = *dynamic_cast<Components::FirstPersonCamera *>(data);
            ImGui::InputFloat("Field of View", &firstPersonCameraComponent.fieldOfView, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Near Clip", &firstPersonCameraComponent.nearClip, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Far Clip", &firstPersonCameraComponent.farClip, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            //ImGui::InputText("Name", firstPersonCameraComponent.range);
            ImGui::SetCurrentContext(nullptr);
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
            population->listEntities<Components::FirstPersonCamera, Components::Transform>([&](Plugin::Entity *entity, const wchar_t *) -> void
            {
                const auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();

                Camera data;
                if (!cameraComponent.name.empty())
                {
                    auto backBuffer = renderer->getDevice()->getBackBuffer();
                    uint32_t width = backBuffer->getWidth();
                    uint32_t height = backBuffer->getHeight();
                    data.target = resources->createTexture(String::create(L"camera:%v", cameraComponent.name), Video::Format::R8G8B8A8_UNORM_SRGB, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                }

                entityDataMap.insert(std::make_pair(entity, data));
            });
        }

        void onLoadFailed(void)
        {
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
