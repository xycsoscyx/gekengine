#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\Float4x4.hpp"
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
            ImGui::InputText("Name", firstPersonCameraComponent.name);
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
        struct Data
        {
            ResourceHandle target;
        };

    private:
        Plugin::Population *population;
        Plugin::Resources *resources;
        Plugin::Renderer *renderer;

        Plugin::ProcessorHelper<Data, Components::FirstPersonCamera, Components::Transform> helper;

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
            helper.clear();
        }

        void onLoadSucceeded(void)
        {
            population->listEntities<Components::FirstPersonCamera, Components::Transform>([&](Plugin::Entity *entity, const wchar_t *) -> void
            {
                helper.addEntity(entity, [&](Data &data) -> void
                {
                    const auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();
                    if (!cameraComponent.name.empty())
                    {
                        auto backBuffer = renderer->getDevice()->getBackBuffer();
                        uint32_t width = backBuffer->getWidth();
                        uint32_t height = backBuffer->getHeight();
                        data.target = resources->createTexture(String::create(L"camera:%v", cameraComponent.name), Video::Format::R8G8B8A8_UNORM_SRGB, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                    }
                });
            });
        }

        void onLoadFailed(void)
        {
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            helper.removeEntity(entity);
        }

        // Plugin::PopulationStep
        void onUpdate(uint32_t order, State state)
        {
            GEK_REQUIRE(renderer);

            if (state != State::Loading)
            {
                helper.list([&](Plugin::Entity *entity, Data &camera) -> void
                {
					const auto &cameraComponent = entity->getComponent<Components::FirstPersonCamera>();
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
