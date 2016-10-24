#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\Matrix4x4.hpp"
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
        , public Plugin::ComponentMixin<Components::FirstPersonCamera, Edit::Component>
    {
    public:
        FirstPersonCamera(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &firstPersonCameraComponent = *dynamic_cast<Components::FirstPersonCamera *>(data);
            ImGui::InputFloat("Field of View", &firstPersonCameraComponent.fieldOfView, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Near Clip", &firstPersonCameraComponent.nearClip, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Far Clip", &firstPersonCameraComponent.farClip, 1.0f, 10.0f, 3, flags);
            ImGui::InputText("Name", firstPersonCameraComponent.name, flags);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"first_person_camera";
        }
    };

    GEK_CONTEXT_USER(CameraProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<CameraProcessor, Components::FirstPersonCamera>
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

            population->onLoadBegin.connect<CameraProcessor, &CameraProcessor::onLoadBegin>(this);
            population->onLoadSucceeded.connect<CameraProcessor, &CameraProcessor::onLoadSucceeded>(this);
            population->onEntityCreated.connect<CameraProcessor, &CameraProcessor::onEntityCreated>(this);
            population->onEntityDestroyed.connect<CameraProcessor, &CameraProcessor::onEntityDestroyed>(this);
            population->onComponentAdded.connect<CameraProcessor, &CameraProcessor::onComponentAdded>(this);
            population->onComponentRemoved.connect<CameraProcessor, &CameraProcessor::onComponentRemoved>(this);
            population->onUpdate[90].connect<CameraProcessor, &CameraProcessor::onUpdate>(this);
        }

        ~CameraProcessor(void)
        {
            population->onUpdate[90].disconnect<CameraProcessor, &CameraProcessor::onUpdate>(this);
            population->onComponentRemoved.disconnect<CameraProcessor, &CameraProcessor::onComponentRemoved>(this);
            population->onComponentAdded.disconnect<CameraProcessor, &CameraProcessor::onComponentAdded>(this);
            population->onEntityDestroyed.disconnect<CameraProcessor, &CameraProcessor::onEntityDestroyed>(this);
            population->onEntityCreated.disconnect<CameraProcessor, &CameraProcessor::onEntityCreated>(this);
            population->onLoadSucceeded.disconnect<CameraProcessor, &CameraProcessor::onLoadSucceeded>(this);
            population->onLoadBegin.disconnect<CameraProcessor, &CameraProcessor::onLoadBegin>(this);
        }

        void addEntity(Plugin::Entity *entity)
        {
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &cameraComponent) -> void
            {
                if (!cameraComponent.name.empty())
                {
                    auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                    uint32_t width = backBuffer->getWidth();
                    uint32_t height = backBuffer->getHeight();
                    data.target = resources->createTexture(String::create(L"camera:%v", cameraComponent.name), Video::Format::R8G8B8A8_UNORM_SRGB, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
                }
            });
        }

        // Plugin::Population Slots
        void onLoadBegin(const String &populationName)
        {
            clear();
        }

        void onLoadSucceeded(const String &populationName)
        {
            population->listEntities([&](Plugin::Entity *entity, const wchar_t *) -> void
            {
                addEntity(entity);
            });
        }

        void onEntityCreated(Plugin::Entity *entity, const wchar_t *entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity *entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity *entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity *entity, const std::type_index &type)
        {
            removeEntity(entity);
        }

        // Plugin::Population Slots
        void onUpdate(void)
        {
            GEK_REQUIRE(renderer);

            list([&](Plugin::Entity *entity, auto &data, auto &cameraComponent) -> void
            {
                const auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                const float width = float(backBuffer->getWidth());
                const float height = float(backBuffer->getHeight());
                Math::Float4x4 projectionMatrix(Math::Float4x4::createPerspective(cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip));

                renderer->render(entity, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, data.target);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
