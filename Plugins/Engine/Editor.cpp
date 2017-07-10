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
#include <set>

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

            int selectedComponent = 0;

            ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
            bool useSnap = true;
            Math::Float3 snapPosition = Math::Float3(1.0f / 12.0f);
            float snapRotation = 10.0f;
            float snapScale = (1.0f / 10.0f);
            float *snapData = nullptr;

            ResourceHandle cameraTarget;
            ImVec2 cameraSize;

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
            void showPopulation(void)
            {
                if (ImGui::BeginDock("Population", nullptr, ImGuiWindowFlags_ShowBorders, ImVec2(100.0f, -1.0f)))
                {
                    if (ImGui::TreeNodeEx("Selection Gizmo", 0))
                    {
                        if (ImGui::RadioButton("Move", currentGizmoOperation == ImGuizmo::TRANSLATE))
                        {
                            currentGizmoOperation = ImGuizmo::TRANSLATE;
                        }

                        ImGui::SameLine();
                        if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
                        {
                            currentGizmoOperation = ImGuizmo::ROTATE;
                        }

                        ImGui::SameLine();
                        if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
                        {
                            currentGizmoOperation = ImGuizmo::SCALE;
                        }

                        ImGui::Checkbox("Snap", &useSnap);
                        ImGui::SameLine();

                        ImGui::PushItemWidth(-1.0f);
                        switch (currentGizmoOperation)
                        {
                        case ImGuizmo::TRANSLATE:
                            ImGui::InputFloat3("##snapTranslation", snapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            snapData = snapPosition.data;
                            break;

                        case ImGuizmo::ROTATE:
                            ImGui::SliderAngle("##snapDegrees", &snapRotation);
                            snapData = &snapRotation;
                            break;

                        case ImGuizmo::SCALE:
                            ImGui::InputFloat("##snapScale", &snapScale, (1.0f / 10.0f), 1.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            snapData = &snapScale;
                            break;
                        };

                        ImGui::PopItemWidth();
                        ImGui::TreePop();
                    }

                    if (ImGui::Button(ICON_MD_ADD_CIRCLE u8"  New Entity", ImVec2(ImGui::GetWindowContentRegionWidth(), 0.0f)))
                    {
                        ImGui::OpenPopup("NewEntity");
                    }

                    if (ImGui::BeginPopup("NewEntity"))
                    {
                        ImGui::Text("New Entity Name:");

                        std::string name;
                        if (UI::InputString("##name", name, ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            population->createEntity(name);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    std::set<Edit::Entity *> deleteEntitySet;
                    auto &entityMap = population->getEntityMap();
                    for (auto &entitySearch : entityMap)
                    {
                        auto &name = entitySearch.first;
                        auto entity = dynamic_cast<Edit::Entity *>(entitySearch.second.get());

                        ImGui::PushID(reinterpret_cast<int>(entity));
                        if (ImGui::Button(ICON_MD_DELETE_FOREVER))
                        {
                            ImGui::OpenPopup("ConfirmEntityDelete");
                        }

                        if (ImGui::BeginPopup("ConfirmEntityDelete"))
                        {
                            ImGui::Text("Are you sure you want to remove this entitiy?");

                            if (ImGui::Button("Yes", ImVec2(50.0f, 25.0f)))
                            {
                                deleteEntitySet.insert(entity);
                                ImGui::CloseCurrentPopup();
                            }

                            ImGui::SameLine();
                            if (ImGui::Button("No", ImVec2(50.0f, 25.0f)))
                            {
                                ImGui::CloseCurrentPopup();
                            }

                            ImGui::EndPopup();
                        }

                        ImGui::PopID();
                        ImGui::SameLine();
                        if (ImGui::TreeNodeEx(name.c_str(), 0))
                        {
                            if (ImGui::Button(ICON_MD_ADD_CIRCLE_OUTLINE u8"  Add Component", ImVec2(ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().IndentSpacing, 0.0f)))
                            {
                                selectedComponent = 0;
                                ImGui::OpenPopup("AddComponent");
                            }

                            if (ImGui::BeginPopup("AddComponent"))
                            {
                                ImGui::Text("Component Type");
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
                                            population->addComponent(entity, componentData);
                                            ImGui::CloseCurrentPopup();
                                        }
                                        }
                                    };

                                    ImGui::ListBoxFooter();
                                }

                                ImGui::EndPopup();
                            }

                            std::set<std::type_index> deleteComponentSet;
                            const auto &entityComponentMap = entity->getComponentMap();
                            for (auto &componentSearch : entityComponentMap)
                            {
                                Edit::Component *component = population->getComponent(componentSearch.first);
                                Plugin::Component::Data *componentData = componentSearch.second.get();
                                if (component && componentData)
                                {
                                    ImGui::PushID(component->getIdentifier().hash_code());
                                    if (ImGui::Button(ICON_MD_DELETE))
                                    {
                                        ImGui::OpenPopup("ConfirmComponentDelete");
                                    }

                                    if (ImGui::BeginPopup("ConfirmComponentDelete"))
                                    {
                                        ImGui::Text("Are you sure you want to remove this component?");

                                        if (ImGui::Button("Yes", ImVec2(50.0f, 25.0f)))
                                        {
                                            ImGui::CloseCurrentPopup();
                                            deleteComponentSet.insert(component->getIdentifier());
                                        }

                                        ImGui::SameLine();
                                        if (ImGui::Button("No", ImVec2(50.0f, 25.0f)))
                                        {
                                            ImGui::CloseCurrentPopup();
                                        }

                                        ImGui::EndPopup();
                                    }

                                    ImGui::PopID();
                                    ImGui::SameLine();
                                    if (ImGui::TreeNodeEx(component->getName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                                    {
                                        if (component->onUserInterface(ImGui::GetCurrentContext(), entity, componentData))
                                        {
                                            onModified.emit(entity, componentSearch.first);
                                        }

                                        ImGui::TreePop();
                                    }
                                }
                            }

                            for (auto &component : deleteComponentSet)
                            {
                                population->removeComponent(entity, component);
                            }

                            ImGui::TreePop();
                        }
                    }

                    for (auto &entity : deleteEntitySet)
                    {
                        population->killEntity(entity);
                    }
                }

                ImGui::EndDock();
            }

            void showScene(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                if (ImGui::BeginDock("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_ShowBorders))
                {
                    cameraSize = ImGui::GetWindowSize();

                    Video::Texture::Description description;
                    description.width = cameraSize.x;
                    description.height = cameraSize.y;
                    description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                    description.format = Video::Format::R11G11B10_FLOAT;
                    cameraTarget = core->getResources()->createTexture("editorTarget", description, Plugin::Resources::Flags::ForceLoad);

                    auto cameraBuffer = dynamic_cast<Engine::Resources *>(core->getResources())->getResource(cameraTarget);
                    if (cameraBuffer)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(cameraBuffer), cameraSize);
                    }

                    Math::Float4x4 viewMatrix(Math::Float4x4::FromPitch(lookingAngle) * Math::Float4x4::FromYaw(headingAngle));
                    viewMatrix.translation.xyz = position;
                    viewMatrix.invert();

                    const auto backBuffer = core->getVideoDevice()->getBackBuffer();
                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 1.0f, 100.0f));

                    ImGuizmo::BeginFrame();
                    ImGuizmo::SetRect(ImGui::GetCursorPosX(), ImGui::GetCursorPosY(), ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
                    auto &entityMap = population->getEntityMap();
                    for (auto &entitySearch : entityMap)
                    {
                        auto &name = entitySearch.first;
                        auto entity = dynamic_cast<Edit::Entity *>(entitySearch.second.get());
                        if (entity->hasComponent<Components::Transform>())
                        {
                            auto &transformComponent = entity->getComponent<Components::Transform>();
                            auto matrix = transformComponent.getMatrix();
                            ImGuizmo::DrawCube(viewMatrix.data, projectionMatrix.data, matrix.data);
                            //ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, currentGizmoOperation, ImGuizmo::WORLD, matrix.data, nullptr, snapData);
                        }
                    }
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

                auto &style = ImGui::GetStyle();
                auto &imGuiIo = ImGui::GetIO();

                auto editorSize = imGuiIo.DisplaySize;
                auto editorPosition = ImVec2(0.0f, 0.0f);
                if (imGuiIo.MouseDrawCursor)
                {
                    editorSize.y -= ImGui::GetItemsLineHeightWithSpacing() - style.ItemSpacing.y;
                    editorPosition.y += ImGui::GetItemsLineHeightWithSpacing() - style.ItemSpacing.y;
                }

                ImGui::SetNextWindowSize(editorSize);
                ImGui::SetNextWindowPos(editorPosition);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                if (ImGui::Begin("Editor", nullptr, editorSize, 1.0f, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
                {
                    ImGui::BeginDockspace();

                    ImGui::SetNextDock(ImGuiDockSlot_None);
                    showScene();

                    ImGui::SetNextDock(ImGuiDockSlot_Right | ImGuiDockSlot_FromRoot);
                    showPopulation();

                    ImGui::EndDockspace();
                }

                ImGui::End();
                ImGui::PopStyleVar();
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

                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 1.0f, 100.0f));

                    renderer->queueCamera(viewMatrix, projectionMatrix, 0.5f, 200.0f, cameraTarget);
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
