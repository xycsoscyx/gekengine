#include "GEK/Math/Common.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/Renderer.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Engine/Entity.hpp"
#include "GEK/Engine/Component.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , public Plugin::Processor
        {
        private:
            Plugin::Core *core = nullptr;
            Edit::Population *population = nullptr;
            Plugin::Renderer *renderer = nullptr;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            Math::Quaternion rotation = Math::Quaternion::Identity;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

            uint32_t selectedEntity = 0;
            uint32_t selectedComponent = 0;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Edit::Population *>(core->getPopulation()))
                , renderer(core->getRenderer())
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(core);

                core->onInterface.connect<Editor, &Editor::onInterface>(this);
                population->onLoadBegin.connect<Editor, &Editor::onLoadBegin>(this);
                population->onAction.connect<Editor, &Editor::onAction>(this);
                population->onUpdate[90].connect<Editor, &Editor::onUpdate>(this);
            }

            ~Editor(void)
            {
                population->onUpdate[90].disconnect<Editor, &Editor::onUpdate>(this);
                population->onAction.disconnect<Editor, &Editor::onAction>(this);
                population->onLoadBegin.disconnect<Editor, &Editor::onLoadBegin>(this);
                core->onInterface.disconnect<Editor, &Editor::onInterface>(this);
            }

            // Plugin::Core Slots
            void onInterface(bool showCursor)
            {
                if (population->isLoading())
                {
                    return;
                }

                auto &configuration = core->getConfiguration();
                bool editingEnabled = configuration[L"editor"][L"enabled"].as_bool();
                bool showSelectionMenu = configuration[L"editor"][L"show_selector"].as_bool();
                if (showCursor && showSelectionMenu && ImGui::BeginDock("Entity List", &showSelectionMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_AlwaysUseWindowPadding))
                {
                    ImGui::Dummy(ImVec2(350, 0));
                    auto &entityMap = population->getEntityMap();
                    if (!entityMap.empty())
                    {
                        ImGui::Separator();
                        if (ImGui::Button("Create Entity", ImVec2(ImGui::GetWindowContentRegionWidth(), 0)))
                        {
                            ImGui::OpenPopup("Entity Name");
                        }

                        if (ImGui::BeginPopup("Entity Name"))
                        {
                            char name[256] = "";
                            if (ImGui::InputText("Name", name, 255, ImGuiInputTextFlags_EnterReturnsTrue))
                            {
                                population->createEntity(String(name));
                                ImGui::CloseCurrentPopup();
                            }

                            ImGui::EndPopup();
                        }

                        ImGui::PushItemWidth(-1.0f);
                        auto entityCount = entityMap.size();
                        if (ImGui::ListBoxHeader("##Entities", entityCount, 7))
                        {
                            ImGuiListClipper clipper(entityCount, ImGui::GetTextLineHeightWithSpacing());
                            while (clipper.Step())
                            {
                                auto entitySearch = std::begin(entityMap);
                                std::advance(entitySearch, clipper.DisplayStart);
                                for (int entityIndex = clipper.DisplayStart; entityIndex < clipper.DisplayEnd; ++entityIndex, ++entitySearch)
                                {
                                    ImGui::PushID(entityIndex);
                                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 0));
                                    if (ImGui::Button("X"))
                                    {
                                        population->killEntity(entitySearch->second.get());
                                    }

                                    ImGui::PopStyleVar();
                                    ImGui::PopID();
                                    ImGui::SameLine();
                                    ImGui::SetItemAllowOverlap();
                                    if (ImGui::Selectable(StringUTF8(entitySearch->first), (entityIndex == selectedEntity)))
                                    {
                                        selectedEntity = entityIndex;
                                        selectedComponent = 0;
                                    }
                                }
                            };

                            ImGui::ListBoxFooter();
                        }

                        ImGui::PopItemWidth();

                        auto entitySearch = std::begin(entityMap);
                        std::advance(entitySearch, selectedEntity);
                        Edit::Entity *entity = dynamic_cast<Edit::Entity *>(entitySearch->second.get());
                        if (entity)
                        {
                            ImGui::Separator();
                            ImGui::PushItemWidth(-1.0f);
                            if (ImGui::Button("Add Component", ImVec2(ImGui::GetWindowContentRegionWidth(), 0)))
                            {
                                ImGui::OpenPopup("Select Component");
                            }

                            if (ImGui::BeginPopup("Select Component"))
                            {
                                auto &componentMap = population->getComponentMap();
                                auto componentCount = componentMap.size();
                                if (ImGui::ListBoxHeader("##Components", componentCount, 7))
                                {
                                    ImGuiListClipper clipper(componentCount, ImGui::GetTextLineHeightWithSpacing());
                                    while (clipper.Step())
                                    {
                                        for (auto componentIndex = clipper.DisplayStart; componentIndex < clipper.DisplayEnd; ++componentIndex)
                                        {
                                            auto componentSearch = std::begin(componentMap);
                                            std::advance(componentSearch, componentIndex);
                                            if (ImGui::Selectable((componentSearch->first.name() + 7), (selectedComponent == componentIndex)))
                                            {
                                                JSON::Member componentData(componentSearch->second->getName(), JSON::Object());
                                                population->addComponent(entity, componentData);
                                                ImGui::CloseCurrentPopup();
                                            }
                                        }
                                    };

                                    ImGui::ListBoxFooter();
                                }

                                ImGui::EndPopup();
                            }

                            auto &entityComponentMap = entity->getComponentMap();
                            if (!entityComponentMap.empty())
                            {
                                auto entityComponentsCount = entityComponentMap.size();
                                if (ImGui::ListBoxHeader("##Components", entityComponentsCount, 7))
                                {
                                    std::vector<std::type_index> componentDeleteList;
                                    ImGuiListClipper clipper(entityComponentsCount, ImGui::GetTextLineHeightWithSpacing());
                                    while (clipper.Step())
                                    {
                                        for (auto componentIndex = clipper.DisplayStart; componentIndex < clipper.DisplayEnd; ++componentIndex)
                                        {
                                            auto entityComponentSearch = std::begin(entityComponentMap);
                                            std::advance(entityComponentSearch, componentIndex);

                                            ImGui::PushID(componentIndex);
                                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 0));
                                            if (ImGui::Button("X"))
                                            {
                                                componentDeleteList.push_back(entityComponentSearch->first);
                                            }

                                            ImGui::PopStyleVar();
                                            ImGui::PopID();
                                            ImGui::SameLine();
                                            ImGui::SetItemAllowOverlap();
                                            if (ImGui::Selectable((entityComponentSearch->first.name() + 7), (selectedComponent == componentIndex)))
                                            {
                                                selectedComponent = componentIndex;
                                            }
                                        }
                                    };

                                    ImGui::ListBoxFooter();
                                    for (auto &componentType : componentDeleteList)
                                    {
                                        population->removeComponent(entity, componentType);
                                    }
                                }

                                ImGui::PopItemWidth();

                                auto entityComponentSearch = std::begin(entityComponentMap);
                                std::advance(entityComponentSearch, selectedComponent);
                                if (entityComponentSearch != std::end(entityComponentMap))
                                {
                                    Edit::Component *component = population->getComponent(entityComponentSearch->first);
                                    Plugin::Component::Data *componentData = entityComponentSearch->second.get();
                                    if (component && componentData)
                                    {
                                        ImGui::Separator();
                                        if (editingEnabled)
                                        {
                                            Math::Float4x4 viewMatrix(rotation, position);
                                            viewMatrix.invert();

                                            const auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                                            const float width = float(backBuffer->getDescription().width);
                                            const float height = float(backBuffer->getDescription().height);
                                            auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));

                                            component->edit(ImGui::GetCurrentContext(), viewMatrix, projectionMatrix, componentData);
                                        }
                                        else
                                        {
                                            component->show(ImGui::GetCurrentContext(), componentData);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ImGui::EndDock();
                }

                configuration[L"editor"][L"show_selector"] = showSelectionMenu;
            }

            // Plugin::Population Slots
            void onAction(const String &actionName, const Plugin::Population::ActionParameter &parameter)
            {
                if (actionName.compareNoCase(L"turn") == 0)
                {
                    headingAngle += (parameter.value * 0.01f);
                }
                else if (actionName.compareNoCase(L"turn") == 0)
                {
                    headingAngle += (parameter.value * 0.01f);
                }
                else if (actionName.compareNoCase(L"move_forward") == 0)
                {
                    moveForward = parameter.state;
                }
                else if (actionName.compareNoCase(L"move_backward") == 0)
                {
                    moveBackward = parameter.state;
                }
                else if (actionName.compareNoCase(L"strafe_left") == 0)
                {
                    strafeLeft = parameter.state;
                }
                else if (actionName.compareNoCase(L"strafe_right") == 0)
                {
                    strafeRight = parameter.state;
                }
            }

            void onLoadBegin(const String &populationName)
            {
                selectedEntity = 0;
                selectedComponent = 0;
                headingAngle = 0.0f;
                lookingAngle = 0.0f;
                position = Math::Float3::Zero;
                rotation = Math::Quaternion::Identity;
                moveForward = false;
                moveBackward = false;
                strafeLeft = false;
                strafeRight = false;
            }

            void onUpdate(void)
            {
                auto &configuration = core->getConfiguration();
                bool editingEnabled = configuration[L"editor"][L"enabled"].as_bool();
                if (editingEnabled)
                {
                    float frameTime = population->getFrameTime();

                    static const Math::Float3 upAxis(0.0f, 1.0f, 0.0f);
                    rotation = Math::Quaternion::FromAngular(upAxis, headingAngle);

                    Math::Float4x4 viewMatrix(rotation);
                    position += (viewMatrix.rz.xyz * (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f) * frameTime);
                    position += (viewMatrix.rx.xyz * (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f) * frameTime);
                    viewMatrix.translation.xyz = position;
                    viewMatrix.invert();

                    const auto backBuffer = renderer->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));

                    renderer->queueRenderCall(viewMatrix, projectionMatrix, 0.5f, 200.0f, nullptr, ResourceHandle());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
