#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Processor.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Renderer.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Math\Common.hpp"
#include "GEK\Math\SIMD\Matrix4x4.hpp"
#include <unordered_map>
#include <ppl.h>

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(FirstPersonCamera)
        {
            float fieldOfView = 90.0f;
            float nearClip = 1.0f;
            float farClip = 100.0f;
            String target;
            std::vector<String> filterList;

            void save(JSON::Object &componentData) const
            {
                componentData.set(L"fieldOfView", Math::RadiansToDegrees(fieldOfView));
				componentData.set(L"nearClip", nearClip);
				componentData.set(L"farClip", farClip);
				componentData.set(L"target", target);
				componentData.set(L"filterList", String::create(filterList, L','));
            }

            void load(const JSON::Object &componentData)
            {
                if (componentData.is_object())
                {
                    fieldOfView = Math::DegreesToRadians(componentData.get(L"fieldOfView", 90.0f).as<float>());
                    nearClip = componentData.get(L"nearClip", 1.0f).as<float>();
                    farClip = componentData.get(L"farClip", 100.0f).as<float>();
                    target = componentData.get(L"target", L"").as_string();
                    filterList = String(componentData.get(L"filterList", L"").as_string()).split(L',');
                }
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
            ImGui::InputText("Target", firstPersonCameraComponent.target, flags);
            if (ImGui::ListBoxHeader("Filters", firstPersonCameraComponent.filterList.size(), 5))
            {
                ImGuiListClipper clipper(firstPersonCameraComponent.filterList.size(), ImGui::GetTextLineHeightWithSpacing());
                while (clipper.Step())
                {
                    for (int filterIndex = clipper.DisplayStart; filterIndex < clipper.DisplayEnd; ++filterIndex)
                    {
                        ImGui::PushID(filterIndex);
                        ImGui::PushItemWidth(-1);
                        ImGui::InputText("##", firstPersonCameraComponent.filterList[filterIndex], flags);
                        ImGui::PopItemWidth();
                        ImGui::PopID();
                    }
                };

                ImGui::ListBoxFooter();
            }

            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
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
            ProcessorMixin::addEntity(entity, [&](auto &data, auto &cameraComponent, auto &transformComponent) -> void
            {
                if (!cameraComponent.target.empty())
                {
                    auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                    uint32_t width = backBuffer->getWidth();
                    uint32_t height = backBuffer->getHeight();
                    data.target = resources->createTexture(String::create(L"camera:%v", cameraComponent.target), Video::Format::R8G8B8A8_UNORM_SRGB, width, height, 1, 1, Video::TextureFlags::RenderTarget | Video::TextureFlags::Resource);
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

            list([&](Plugin::Entity *entity, auto &data, auto &cameraComponent, auto &transformComponent) -> void
            {
                auto viewMatrix(transformComponent.getMatrix().getInverse());

                const auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                const float width = float(backBuffer->getWidth());
                const float height = float(backBuffer->getHeight());
                Math::SIMD::Float4x4 projectionMatrix(Math::SIMD::Float4x4::MakePerspective(cameraComponent.fieldOfView, (width / height), cameraComponent.nearClip, cameraComponent.farClip));

                renderer->render(viewMatrix, projectionMatrix, cameraComponent.nearClip, cameraComponent.farClip, &cameraComponent.filterList, data.target);
            });
        }
    };

    GEK_REGISTER_CONTEXT_USER(FirstPersonCamera);
    GEK_REGISTER_CONTEXT_USER(CameraProcessor);
}; // namespace Gek
