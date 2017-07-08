#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include <unordered_map>
#include <ppl.h>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(FirstPersonCamera)
        {
            float fieldOfView = Math::DegreesToRadians(90.0f);
            float nearClip = 1.0f;
            float farClip = 100.0f;
            std::string target;
        };
    };

    GEK_CONTEXT_USER(FirstPersonCamera, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::FirstPersonCamera, Edit::Component>
    {
    public:
        FirstPersonCamera(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::FirstPersonCamera const * const data, JSON::Object &componentData) const
        {
            componentData["fieldOfView"] = Math::RadiansToDegrees(data->fieldOfView);
            componentData["nearClip"] = data->nearClip;
            componentData["farClip"] = data->farClip;
            componentData["target"] = data->target;
        }

        void load(Components::FirstPersonCamera * const data, JSON::Reference componentData)
        {
            data->fieldOfView = Math::DegreesToRadians(parse(componentData.get("fieldOfView"), 90.0f));
            data->nearClip = parse(componentData.get("nearClip"), 1.0f);
            data->farClip = parse(componentData.get("farClip"), 100.0f);
            data->target = parse(componentData.get("target"), String::Empty);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &firstPersonCameraComponent = *dynamic_cast<Components::FirstPersonCamera *>(data);

            changed |= editorElement("Field of View", [&](void) -> bool
            {
                return ImGui::SliderAngle("##fieldOfView", &firstPersonCameraComponent.fieldOfView, 0.0f, 180.0f);
            });

            changed |= editorElement("Near Clip", [&](void) -> bool
            {
                return ImGui::InputFloat("##nearClip", &firstPersonCameraComponent.nearClip, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Far Clip", [&](void) -> bool
            {
                return ImGui::InputFloat("##farClip", &firstPersonCameraComponent.farClip, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Target", [&](void) -> bool
            {
                return UI::InputString("##target", firstPersonCameraComponent.target, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_CONTEXT_USER(CameraProcessor, Plugin::Core *)
        , public Plugin::ProcessorMixin<CameraProcessor, Components::FirstPersonCamera, Components::Transform>
        , public Plugin::Processor
    {
    public:
        struct Data
        {
            ResourceHandle target;
        };

    private:
        Plugin::Core *core = nullptr;
        Plugin::Population *population = nullptr;
        Plugin::Resources *resources = nullptr;
        Plugin::Renderer *renderer = nullptr;

    public:
        CameraProcessor(Context *context, Plugin::Core *core)
            : ContextRegistration(context)
            , core(core)
            , population(core->getPopulation())
            , resources(core->getResources())
            , renderer(core->getRenderer())
        {
            assert(population);
            assert(resources);
            assert(renderer);

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
        }

        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &cameraComponent, auto &transformComponent) -> void
            {
                if (!cameraComponent.target.empty())
                {
                    auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    Video::Texture::Description description;
                    description.format = Video::Format::R8G8B8A8_UNORM_SRGB;
                    description.width = backBuffer->getDescription().width;
                    description.height = backBuffer->getDescription().height;
                    description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                    data.target = resources->createTexture(String::Format("camera:%v", cameraComponent.target), description);
                }
            });
        }

        // Plugin::Population Slots
        void onEntityCreated(Plugin::Entity * const entity, std::string const &entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        // Plugin::Population Slots
        void onUpdate(float frameTime)
        {
            assert(renderer);

            bool editorActive = core->getOption("editor", "active").convert(false);
            if (frameTime > 0.0f && !editorActive)
            {
                parallelListEntities([&](Plugin::Entity * const entity, auto &data, auto &cameraComponent, auto &transformComponent) -> void
                {
                    auto viewMatrix(transformComponent.getMatrix().getInverse());

                    const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    Math::Float4x4 projectionMatrix(Math::Float4x4::MakePerspective(cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip));

                    renderer->queueCamera(viewMatrix, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, data.target);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
