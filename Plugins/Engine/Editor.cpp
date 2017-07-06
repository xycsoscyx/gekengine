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
#include "GEK/Engine/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , public Plugin::Processor
            , public Plugin::Editor
        {
        private:
            Plugin::Core *core = nullptr;
            Edit::Population *population = nullptr;
            Plugin::Renderer *renderer = nullptr;

            Video::TexturePtr dockPanelIcon;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Edit::Population *>(core->getPopulation()))
                , renderer(core->getRenderer())
            {
                assert(population);
                assert(core);

                core->setOption("editor", "active", false);

                population->onAction.connect<Editor, &Editor::onAction>(this);
                population->onUpdate[90].connect<Editor, &Editor::onUpdate>(this);

                renderer->onShowUserInterface.connect<Editor, &Editor::onShowUserInterface>(this);

                if (!ImGui::TabWindow::DockPanelIconTextureID)
                {
                    int iconSize = 0;
                    void const *iconBuffer = ImGui::TabWindow::GetDockPanelIconImagePng(&iconSize);
                    dockPanelIcon = core->getVideoDevice()->loadTexture(iconBuffer, iconSize, 0);
                    ImGui::TabWindow::DockPanelIconTextureID = dynamic_cast<Video::Object *>(dockPanelIcon.get());
                }
            }

            ~Editor(void)
            {
                renderer->onShowUserInterface.disconnect<Editor, &Editor::onShowUserInterface>(this);

                population->onUpdate[90].disconnect<Editor, &Editor::onUpdate>(this);
                population->onAction.disconnect<Editor, &Editor::onAction>(this);

                ImGui::ShutdownDock();
            }

            // Renderer
            Edit::Entity *selectedEntity = nullptr;
            void showEntities(void)
            {
                if (ImGui::BeginDock("Entities", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize, ImVec2(100.0f, -1.0f)))
                {
                    if (ImGui::Button("Create Entity", ImVec2(ImGui::GetWindowContentRegionWidth(), 0)))
                    {
                        ImGui::OpenPopup("Entity Name");
                    }

                    if (ImGui::BeginPopup("Entity Name"))
                    {
                        std::string name;
                        if (GUI::InputString("Name", name, ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            population->createEntity(name);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    auto &entityMap = population->getEntityMap();
                    for (auto &entitySearch : entityMap)
                    {
                        ImGui::PushID(entitySearch.second.get());
                        if (ImGui::Button("X", ImVec2(9, 9)))
                        {
                            population->killEntity(entitySearch.second.get());
                        }

                        ImGui::PopID();
                        ImGui::SameLine();
                        ImGui::SetItemAllowOverlap();
                        if (ImGui::Selectable(entitySearch.first.c_str(), (entitySearch.second.get() == selectedEntity)))
                        {
                            selectedEntity = dynamic_cast<Edit::Entity *>(entitySearch.second.get());
                        }
                    }
                }

                ImGui::EndDock();
            }

            int selectedComponent = 0;
            void showComponents(void)
            {
                const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                const float width = float(backBuffer->getDescription().width);
                const float height = float(backBuffer->getDescription().height);
                const auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));

                Math::Float4x4 viewMatrix(Math::Float4x4::FromPitch(lookingAngle) * Math::Float4x4::FromYaw(headingAngle));
                viewMatrix.translation.xyz = position;
                viewMatrix.invert();

                if (ImGui::BeginDock("Components", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize, ImVec2(100.0f, -1.0f)))
                {
                    if (selectedEntity)
                    {
                        if (ImGui::Button("Add Component", ImVec2(ImGui::GetWindowContentRegionWidth(), 0)))
                        {
                            ImGui::OpenPopup("Select Component");
                        }

                        if (ImGui::BeginPopup("Select Component"))
                        {
                            const auto &componentMap = population->getComponentMap();
                            const auto componentCount = componentMap.size();
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
                                            auto componentData = std::make_pair(componentSearch->second->getName(), JSON::EmptyObject);
                                            population->addComponent(selectedEntity, componentData);
                                            ImGui::CloseCurrentPopup();
                                        }
                                    }
                                };

                                ImGui::ListBoxFooter();
                            }

                            ImGui::EndPopup();
                        }

                        const auto &entityComponentMap = selectedEntity->getComponentMap();
                        for (auto &componentSearch : entityComponentMap)
                        {
                            if (ImGui::BeginChildFrame(componentSearch.first.hash_code(), ImVec2(0.0f, 0.0f), ImGuiWindowFlags_AlwaysAutoResize))
                            {
                                ImGui::Text(componentSearch.first.name() + 7);
                                Edit::Component *component = population->getComponent(componentSearch.first);
                                Plugin::Component::Data *componentData = componentSearch.second.get();
                                if (component && componentData)
                                {
                                    ImGui::PushItemWidth(-1.0f);
                                    if (component->edit(ImGui::GetCurrentContext(), viewMatrix, projectionMatrix, selectedEntity, componentData))
                                    {
                                        onModified.emit(selectedEntity, componentSearch.first);
                                    }

                                    ImGui::PopItemWidth();
                                }
                            }

                            ImGui::EndChildFrame();
                        }
                    }
                }

                ImGui::EndDock();
            }

            void showTools(void)
            {
                if (ImGui::BeginDock("Tools", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize, ImVec2(-1.0f, 50.0f)))
                {
                    ImGui::Button("Tools");
                }

                ImGui::EndDock();
            }

            void showResources(void)
            {
                if (ImGui::BeginDock("Resources", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize, ImVec2(-1.0f, 100.0f)))
                {
                    ImGui::Button("Resources");
                }

                ImGui::EndDock();
            }

            void showScene(void)
            {
                if (ImGui::BeginDock("Scene", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize, ImVec2(-1.0f, -1.0f)))
                {
                    ImGui::Button("Scene");
                }

                ImGui::EndDock();
            }

            void onShowUserInterface(ImGuiContext * const guiContext)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (!editorActive)
                {
                    return;
                }

                ImGuiIO &imGuiIo = ImGui::GetIO();
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.0f, 3.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                if (ImGui::Begin("Editor", nullptr, imGuiIo.DisplaySize, 0.75f, ImGuiWindowFlags_NoTitleBar | (imGuiIo.MouseDrawCursor ? ImGuiWindowFlags_MenuBar : 0) | ImGuiWindowFlags_NoSavedSettings))
                {
                    ImGui::BeginDockspace();

                    ImGui::SetNextDock(ImGuiDockSlot_None);
                    showScene();
                    ImGui::SetNextDock(ImGuiDockSlot_Left | ImGuiDockSlot_FromRoot);
                    showEntities();
                    ImGui::SetNextDock(ImGuiDockSlot_Right | ImGuiDockSlot_FromRoot);
                    showComponents();
                    ImGui::SetNextDock(ImGuiDockSlot_Top | ImGuiDockSlot_FromRoot);
                    showTools();
                    ImGui::SetNextDock(ImGuiDockSlot_Bottom | ImGuiDockSlot_FromRoot);
                    showResources();

                    ImGui::EndDockspace();
                }

                ImGui::End();
                ImGui::PopStyleVar(2);
            }

            // Plugin::Population Slots
            void onAction(Plugin::Population::Action const &action)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (!editorActive)
                {
                    return;
                }

                if (action.name == "turn")
                {
                    headingAngle += (action.value * 0.01f);
                }
                else if (action.name == "tilt")
                {
                    lookingAngle += (action.value * 0.01f);
                    lookingAngle = Math::Clamp(lookingAngle, -Math::Pi * 0.5f, Math::Pi * 0.5f);
                }
                else if (action.name == "move_forward")
                {
                    moveForward = action.state;
                }
                else if (action.name == "move_backward")
                {
                    moveBackward = action.state;
                }
                else if (action.name == "strafe_left")
                {
                    strafeLeft = action.state;
                }
                else if (action.name == "strafe_right")
                {
                    strafeRight = action.state;
                }
            }

            void onUpdate(float frameTime)
            {
                bool editorActive = core->getOption("editor", "active").convert(false);
                if (editorActive)
                {
                    Math::Float4x4 viewMatrix(Math::Float4x4::FromPitch(lookingAngle) * Math::Float4x4::FromYaw(headingAngle));
                    position += (viewMatrix.rz.xyz * (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f) * frameTime);
                    position += (viewMatrix.rx.xyz * (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f) * frameTime);
                    viewMatrix.translation.xyz = position;
                    viewMatrix.invert();

                    const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    const float width = float(backBuffer->getDescription().width);
                    const float height = float(backBuffer->getDescription().height);
                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));

                    renderer->queueCamera(viewMatrix, projectionMatrix, 0.5f, 200.0f, ResourceHandle());
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
