#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Processor.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Name.hpp"
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
            float nearClip = 0.1f;
            float farClip = 200.0f;
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

            changed |= editorElement("Clip Range", [&](void) -> bool
            {
                return ImGui::DragFloatRange2("##clipRange", &firstPersonCameraComponent.nearClip, &firstPersonCameraComponent.farClip);
            });

            changed |= editorElement("Target", [&](void) -> bool
            {
                return UI::InputString("##target", firstPersonCameraComponent.target, ImGuiInputTextFlags_EnterReturnsTrue);
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

            core->onShutdown.connect(this, &CameraProcessor::onShutdown);
            population->onReset.connect(this, &CameraProcessor::onReset);
            population->onEntityCreated.connect(this, &CameraProcessor::onEntityCreated);
            population->onEntityDestroyed.connect(this, &CameraProcessor::onEntityDestroyed);
            population->onComponentAdded.connect(this, &CameraProcessor::onComponentAdded);
            population->onComponentRemoved.connect(this, &CameraProcessor::onComponentRemoved);
            population->onUpdate[90].connect(this, &CameraProcessor::onUpdate);
        }

        void addEntity(Plugin::Entity * const entity)
        {
            ProcessorMixin::addEntity(entity, [&](bool isNewInsert, auto &data, auto &cameraComponent, auto &transformComponent) -> void
            {
                if (!cameraComponent.target.empty())
                {
                    auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    Video::Texture::Description description;
                    description.format = Video::Format::R11G11B10_FLOAT;
                    description.width = backBuffer->getDescription().width;
                    description.height = backBuffer->getDescription().height;
                    description.flags = Video::Texture::Flags::RenderTarget | Video::Texture::Flags::Resource;
                    data.target = resources->createTexture(String::Format("camera:{}", cameraComponent.target), description);
                }
            });
        }

        // Plugin::Core
        void onShutdown(void)
        {
            population->onReset.disconnect(this, &CameraProcessor::onReset);
            population->onEntityCreated.disconnect(this, &CameraProcessor::onEntityCreated);
            population->onEntityDestroyed.disconnect(this, &CameraProcessor::onEntityDestroyed);
            population->onComponentAdded.disconnect(this, &CameraProcessor::onComponentAdded);
            population->onComponentRemoved.disconnect(this, &CameraProcessor::onComponentRemoved);
            population->onUpdate[90].disconnect(this, &CameraProcessor::onUpdate);
            clear();
        }

        // Plugin::Population Slots
        void onReset(void)
        {
            clear();
        }

        void onEntityCreated(Plugin::Entity * const entity)
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
                    std::string name;
                    if (entity->hasComponent<Components::Name>())
                    {
                        name = entity->getComponent<Components::Name>().name;
                    }
                    else
                    {
                        name = String::Format("camera_{}", *reinterpret_cast<int *>(entity));
                    }

                    auto viewMatrix(transformComponent.getMatrix().getInverse());

                    const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    renderer->queueCamera(viewMatrix, cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip, name, data.target);
                });
            }
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
