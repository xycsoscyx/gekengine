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
#include "GEK/Components/Name.hpp"
#include "GEK/Model/Base.hpp"
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
            Gek::Processor::Model *modelProcessor = nullptr;

            Video::TexturePtr dockPanelIcon;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

            int selectedComponent = 0;

            int currentGizmoScope = ImGuizmo::WORLD;
            int currentGizmoOperation = ImGuizmo::TRANSLATE;
            bool useGizmoSnap = true;
            Math::Float3 gizmoSnapPosition = Math::Float3(1.0f);
            float gizmoSnapRotation = 10.0f;
            float gizmoSnapScale = 1.0f;

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

            // Plugin::Processor
            void onInitialized(void)
            {
                core->listProcessors([&](Plugin::Processor *processor) -> void
                {
                    auto check = dynamic_cast<Gek::Processor::Model *>(processor);
                    if (check)
                    {
                        modelProcessor = check;
                    }
                });
            }

            // Renderer
            Plugin::Entity *selectedEntity = nullptr;
            void showPopulation(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                if (ImGui::BeginDock("Population", nullptr, 0, ImVec2(imGuiIo.DisplaySize.x * 0.2f, -1.0f)))
                {
                    ImGui::Dummy(ImVec2(50.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::RadioButton("World", &currentGizmoScope, ImGuizmo::WORLD);
                    ImGui::SameLine();
                    ImGui::RadioButton("Local", &currentGizmoScope, ImGuizmo::LOCAL);

                    ImGui::Dummy(ImVec2(30.0f, 0.0f));
                    ImGui::SameLine();
                    ImGui::RadioButton("Move", &currentGizmoOperation, ImGuizmo::TRANSLATE);
                    ImGui::SameLine();
                    ImGui::RadioButton("Rotate", &currentGizmoOperation, ImGuizmo::ROTATE);
                    ImGui::SameLine();
                    ImGui::RadioButton("Scale", &currentGizmoOperation, ImGuizmo::SCALE);

                    ImGui::Checkbox("Snap", &useGizmoSnap);
                    ImGui::SameLine();

                    ImGui::PushItemWidth(-1.0f);
                    switch (currentGizmoOperation)
                    {
                    case ImGuizmo::TRANSLATE:
                        ImGui::InputFloat3("##snapTranslation", gizmoSnapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        break;

                    case ImGuizmo::ROTATE:
                        ImGui::SliderAngle("##snapDegrees", &gizmoSnapRotation);
                        break;

                    case ImGuizmo::SCALE:
                        ImGui::InputFloat("##gizmoSnapScale", &gizmoSnapScale, (1.0f / 10.0f), 1.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        break;
                    };

                    ImGui::PopItemWidth();

                    ImGui::Separator();

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
                            std::vector<Plugin::Population::Component> componentList;
                            if (!name.empty())
                            {
                                componentList.push_back(std::make_pair("Name", name));
                            }

                            population->createEntity(componentList);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    std::set<Edit::Entity *> deleteEntitySet;
                    for (const auto &entity : population->getEntityList())
                    {
                        auto editorEntity = dynamic_cast<Edit::Entity *>(entity.get());
                        auto name = String::Format("entity_%v", entity.get());
                        if (entity->hasComponent<Components::Name>())
                        {
                            name = entity->getComponent<Components::Name>().name;
                        }

                        ImGui::PushID(reinterpret_cast<int>(editorEntity));
                        if (ImGui::RadioButton("##select", selectedEntity == entity.get()))
                        {
                            selectedEntity = entity.get();
                        }

                        ImGui::PopID();
                        ImGui::SameLine();
                        ImGui::PushID(reinterpret_cast<int>(editorEntity));
                        if (ImGui::Button(ICON_MD_DELETE_FOREVER))
                        {
                            ImGui::OpenPopup("ConfirmEntityDelete");
                        }

                        if (ImGui::BeginPopup("ConfirmEntityDelete"))
                        {
                            ImGui::Text("Are you sure you want to remove this entitiy?");

                            if (ImGui::Button("Yes", ImVec2(50.0f, 25.0f)))
                            {
                                deleteEntitySet.insert(editorEntity);
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
                        ImGui::SetNextTreeNodeOpen(selectedEntity == entity.get());
                        if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Framed))
                        {
                            selectedEntity = entity.get();
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
                                                population->addComponent(entity.get(), componentData);
                                                ImGui::CloseCurrentPopup();
                                            }
                                        }
                                    };

                                    ImGui::ListBoxFooter();
                                }

                                ImGui::EndPopup();
                            }

                            std::set<std::type_index> deleteComponentSet;
                            const auto &entityComponentMap = editorEntity->getComponentMap();
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
                                    if (ImGui::TreeNodeEx(component->getName().c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                                    {
                                        if (component->onUserInterface(ImGui::GetCurrentContext(), entity.get(), componentData))
                                        {
                                            onModified.emit(entity.get(), componentSearch.first);
                                        }

                                        ImGui::TreePop();
                                    }
                                }
                            }

                            for (auto &component : deleteComponentSet)
                            {
                                population->removeComponent(entity.get(), component);
                            }

                            ImGui::TreePop();
                        }

                        if (!ImGui::IsItemVisible())
                        {
                            break;
                        }
                    }

                    for (auto &entity : deleteEntitySet)
                    {
                        if (selectedEntity == entity)
                        {
                            selectedEntity = nullptr;
                        }

                        population->killEntity(entity);
                    }
                }

                ImGui::EndDock();
            }

            bool isObjectInFrustum(Shapes::Frustum &frustum, Shapes::OrientedBox &orientedBox)
            {
                for (auto &plane : frustum.planeList)
                {
                    float distance = plane.getDistance(orientedBox.matrix.translation.xyz);
                    float radiusX = std::abs(orientedBox.matrix.rx.xyz.dot(plane.normal) * orientedBox.halfsize.x);
                    float radiusY = std::abs(orientedBox.matrix.ry.xyz.dot(plane.normal) * orientedBox.halfsize.y);
                    float radiusZ = std::abs(orientedBox.matrix.rz.xyz.dot(plane.normal) * orientedBox.halfsize.z);
                    float radius = (radiusX + radiusY + radiusZ);
                    if (distance < -radius)
                    {
                        return false;
                    }
                }

                return true;
            }

            void showScene(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                if (ImGui::BeginDock("Scene", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse, ImVec2(imGuiIo.DisplaySize.x * 0.8f, -1.0f)))
                {
                    cameraSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin();

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
                        auto origin = ImGui::GetItemRectMin();
                        auto size = ImGui::GetItemRectSize();

                        Math::Float4x4 viewMatrix(Math::Float4x4::FromPitch(lookingAngle) * Math::Float4x4::FromYaw(headingAngle));
                        viewMatrix.translation.xyz = position;
                        viewMatrix.invert();

                        auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f));

                        ImGuizmo::BeginFrame();
                        ImGuizmo::SetRect(origin.x, origin.y, size.x, size.y);
                        ImGui::PushClipRect(ImVec2(origin.x, origin.y), ImVec2(origin.x + size.x, origin.y + size.y), false);
                        for (const auto &entity : population->getEntityList())
                        {
                            auto editorEntity = dynamic_cast<Edit::Entity *>(entity.get());
                            if (editorEntity->hasComponent<Components::Transform>())
                            {
                                auto &transformComponent = entity->getComponent<Components::Transform>();
                                auto matrix = transformComponent.getMatrix();
                                if (selectedEntity == entity.get())
                                {
                                    float *snapData = nullptr;
                                    if (useGizmoSnap)
                                    {
                                        switch (currentGizmoOperation)
                                        {
                                        case ImGuizmo::TRANSLATE:
                                            snapData = gizmoSnapPosition.data;
                                            break;

                                        case ImGuizmo::ROTATE:
                                            snapData = &gizmoSnapRotation;
                                            break;

                                        case ImGuizmo::SCALE:
                                            snapData = &gizmoSnapScale;
                                            break;
                                        };
                                    }
                                    
                                    Shapes::AlignedBox boundingBox(0.5f);
                                    float *localbounds = boundingBox.minimum.data;
                                    if (entity->hasComponent<Components::Model>())
                                    {
                                        auto &modelComponent = entity->getComponent<Components::Model>();
                                        boundingBox = modelProcessor->getBoundingBox(modelComponent.name);
                                    }

                                    if (isObjectInFrustum(Shapes::Frustum(viewMatrix * projectionMatrix), Shapes::OrientedBox(boundingBox, matrix)))
                                    {
                                        Math::Float4x4 deltaMatrix;
                                        ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, static_cast<ImGuizmo::OPERATION>(currentGizmoOperation), static_cast<ImGuizmo::MODE>(currentGizmoScope), matrix.data, deltaMatrix.data, snapData, localbounds);
                                        if (memcmp(deltaMatrix.data, Math::Float4x4::Identity.data, sizeof(Math::Float4x4)) != 0)
                                        {
                                            switch (currentGizmoOperation)
                                            {
                                            case ImGuizmo::TRANSLATE:
                                                transformComponent.position = matrix.translation.xyz;
                                                break;

                                            case ImGuizmo::ROTATE:
                                                transformComponent.rotation = matrix.getRotation();
                                                break;

                                            case ImGuizmo::SCALE:
                                                transformComponent.scale = matrix.getScaling();
                                                break;
                                            };

                                            onModified.emit(selectedEntity, typeid(Components::Transform));
                                        }
                                    }
                                }
                                else
                                {
                                    //ImGuizmo::DrawCube(viewMatrix.data, projectionMatrix.data, matrix.data);
                                }
                            }
                        }

                        ImGui::PopClipRect();
                    }
                }

                ImGui::EndDock();
                ImGui::PopStyleVar(2);
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
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                auto oldWindowPadding = ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                if (ImGui::Begin("Editor", nullptr, editorSize, 1.0f, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, oldWindowPadding);
                    ImGui::BeginDockspace("##Editor", (ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin()), true);

                    ImGui::SetNextDock(ImGuiDockSlot_Left | ImGuiDockSlot_FromRoot);
                    showScene();

                    ImGui::SetNextDock(ImGuiDockSlot_Right | ImGuiDockSlot_FromRoot);
                    showPopulation();

                    ImGui::EndDockspace();
                    ImGui::PopStyleVar(1);
                }

                ImGui::End();
                ImGui::PopStyleVar(3);
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

                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f));

                    renderer->queueCamera(viewMatrix, projectionMatrix, 0.1f, 200.0f, cameraTarget);
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
