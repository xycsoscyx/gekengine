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
            WString target;
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
            componentData.set(L"fieldOfView", Math::RadiansToDegrees(data->fieldOfView));
            componentData.set(L"nearClip", data->nearClip);
            componentData.set(L"farClip", data->farClip);
            componentData.set(L"target", data->target);
        }

        void load(Components::FirstPersonCamera * const data, const JSON::Object &componentData)
        {
            data->fieldOfView = Math::DegreesToRadians(getValue(componentData, L"fieldOfView", 90.0f));
            data->nearClip = getValue(componentData, L"nearClip", 1.0f);
            data->farClip = getValue(componentData, L"farClip", 100.0f);
            data->target = getValue(componentData, L"target", WString());
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &firstPersonCameraComponent = *dynamic_cast<Components::FirstPersonCamera *>(data);
            bool changed =
                ImGui::Gek::InputFloat("Field of View", &firstPersonCameraComponent.fieldOfView, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                ImGui::Gek::InputFloat("Near Clip", &firstPersonCameraComponent.nearClip, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                ImGui::Gek::InputFloat("Far Clip", &firstPersonCameraComponent.farClip, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                ImGui::Gek::InputString("Target", firstPersonCameraComponent.target, flags);
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
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
            GEK_REQUIRE(population);
            GEK_REQUIRE(resources);
            GEK_REQUIRE(renderer);

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
                    description.sampleCount = 4;
                    description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                    data.target = resources->createTexture(WString::Format(L"camera:%v", cameraComponent.target), description);
                }
            });
        }

        // Plugin::Population Slots
        void onEntityCreated(Plugin::Entity * const entity, wchar_t const * const entityName)
        {
            addEntity(entity);
        }

        void onEntityDestroyed(Plugin::Entity * const entity)
        {
            removeEntity(entity);
        }

        void onComponentAdded(Plugin::Entity * const entity, const std::type_index &type)
        {
            addEntity(entity);
        }

        void onComponentRemoved(Plugin::Entity * const entity, const std::type_index &type)
        {
            removeEntity(entity);
        }

        // Plugin::Population Slots
        void onUpdate(float frameTime)
        {
            GEK_REQUIRE(renderer);

            if (frameTime > 0.0f && !core->isEditorActive())
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
