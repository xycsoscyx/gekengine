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
            String target;
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
        void save(const Components::FirstPersonCamera *data, JSON::Object &componentData) const
        {
            componentData.set(L"fieldOfView", Math::RadiansToDegrees(data->fieldOfView));
            componentData.set(L"nearClip", data->nearClip);
            componentData.set(L"farClip", data->farClip);
            componentData.set(L"target", data->target);
        }

        void load(Components::FirstPersonCamera *data, const JSON::Object &componentData)
        {
            data->fieldOfView = Math::DegreesToRadians(getValue(componentData, L"fieldOfView", 90.0f));
            data->nearClip = getValue(componentData, L"nearClip", 1.0f);
            data->farClip = getValue(componentData, L"farClip", 100.0f);
            data->target = getValue(componentData, L"target", String());
        }

        // Edit::Component
        bool ui(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data, uint32_t flags)
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

        void show(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Entity *entity, Plugin::Component::Data *data)
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
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &cameraComponent, auto &transformComponent) -> void
            {
                if (!cameraComponent.target.empty())
                {
                    auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                    Video::TextureDescription description;
                    description.format = Video::Format::R8G8B8A8_UNORM_SRGB;
                    description.width = backBuffer->getDescription().width;
                    description.height = backBuffer->getDescription().height;
                    description.sampleCount = 4;
                    description.flags = Video::TextureDescription::Flags::RenderTarget | Video::TextureDescription::Flags::Resource;
                    data.target = resources->createTexture(String::Format(L"camera:%v", cameraComponent.target), description);
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

            if (!core->isEditorActive())
            {
                list([&](Plugin::Entity *entity, auto &data, auto &cameraComponent, auto &transformComponent) -> void
                {
                    auto viewMatrix(transformComponent.getMatrix().getInverse());

                    const auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    Math::Float4x4 projectionMatrix(Math::Float4x4::MakePerspective(cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip));

                    renderer->queueRenderCall(viewMatrix, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, data.target);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
