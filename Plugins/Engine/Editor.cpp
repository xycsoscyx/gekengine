#include "GEK/Math/Common.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/GUI/Dock.hpp"
#include "GEK/GUI/Gizmo.hpp"
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

            std::unique_ptr<UI::Dock::WorkSpace> dock;
            std::unique_ptr<UI::Gizmo::WorkSpace> gizmo;
            Video::TexturePtr dockPanelIcon;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

            int selectedComponent = 0;

            UI::Gizmo::LockAxis currentGizmoAxis = UI::Gizmo::LockAxis::Automatic;
            UI::Gizmo::Alignment currentGizmoAlignment = UI::Gizmo::Alignment::Local;
            UI::Gizmo::Operation currentGizmoOperation = UI::Gizmo::Operation::Translate;
            bool useGizmoSnap = true;
            Math::Float3 gizmoSnapPosition = Math::Float3::One;
            float gizmoSnapRotation = 10.0f;
            float gizmoSnapScale = 1.0f;
            Math::Float3 gizmoSnapBounds = Math::Float3::One;

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

                if (!ImGui::TabWindow::DockPanelIconTextureID)
                {
                    int iconSize = 0;
                    void const *iconBuffer = ImGui::TabWindow::GetDockPanelIconImagePng(&iconSize);
                    dockPanelIcon = core->getVideoDevice()->loadTexture(iconBuffer, iconSize, 0);
                    ImGui::TabWindow::DockPanelIconTextureID = dynamic_cast<Video::Object *>(dockPanelIcon.get());
                }

                dock = std::make_unique<UI::Dock::WorkSpace>();
                gizmo = std::make_unique<UI::Gizmo::WorkSpace>();
                population->onAction.connect(this, &Editor::onAction);
                population->onUpdate[90].connect(this, &Editor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Editor::onShowUserInterface);
            }

            ~Editor(void)
            {
                gizmo = nullptr;
                dock = nullptr;
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

            void onDestroyed(void)
            {
                population->onAction.disconnect(this, &Editor::onAction);
                population->onUpdate[90].disconnect(this, &Editor::onUpdate);
                renderer->onShowUserInterface.disconnect(this, &Editor::onShowUserInterface);
            }

            // Renderer
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

            bool showSceneDock = true;
            void showScene(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                if (dock->BeginTab("Scene", &showSceneDock, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_ShowBorders))
                {
                    cameraSize = UI::GetWindowContentRegionSize();

                    Video::Texture::Description description;
                    description.width = cameraSize.x;
                    description.height = cameraSize.y;
                    description.flags = Video::Texture::Description::Flags::RenderTarget | Video::Texture::Description::Flags::Resource;
                    description.format = Video::Format::R11G11B10_FLOAT;
                    cameraTarget = core->getResources()->createTexture("editorTarget", description, Plugin::Resources::Flags::LoadImmediately);

                    auto cameraBuffer = dynamic_cast<Engine::Resources *>(core->getResources())->getResource(cameraTarget);
                    if (cameraBuffer)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(cameraBuffer), cameraSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                        if (selectedEntity)
                        {
                            if (selectedEntity->hasComponent<Components::Transform>())
                            {
                                auto &transformComponent = selectedEntity->getComponent<Components::Transform>();
                                auto matrix = transformComponent.getScaledMatrix();
                                float *snapData = nullptr;
                                if (useGizmoSnap)
                                {
                                    switch (currentGizmoOperation)
                                    {
                                    case UI::Gizmo::Operation::Translate:
                                        snapData = gizmoSnapPosition.data;
                                        break;

                                    case UI::Gizmo::Operation::Rotate:
                                        snapData = &gizmoSnapRotation;
                                        break;

                                    case UI::Gizmo::Operation::Scale:
                                        snapData = &gizmoSnapScale;
                                        break;

                                    case UI::Gizmo::Operation::Bounds:
                                        snapData = gizmoSnapBounds.data;
                                        break;
                                    };
                                }

                                Shapes::AlignedBox boundingBox(0.5f);
                                if (selectedEntity->hasComponent<Components::Model>())
                                {
                                    auto &modelComponent = selectedEntity->getComponent<Components::Model>();
                                    boundingBox = modelProcessor->getBoundingBox(modelComponent.name);
                                }

                                auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f));
                                Math::Float4x4 viewMatrix(Math::Float4x4::MakePitchRotation(lookingAngle) * Math::Float4x4::MakeYawRotation(headingAngle));
                                viewMatrix.translation.xyz = position;
                                viewMatrix.invert();

                                if (isObjectInFrustum(Shapes::Frustum(viewMatrix * projectionMatrix), Shapes::OrientedBox(matrix, boundingBox)))
                                {
                                    Math::Float4x4 deltaMatrix;
                                    auto size = ImGui::GetItemRectSize();
                                    auto origin = ImGui::GetItemRectMin();
                                    gizmo->beginFrame(viewMatrix, projectionMatrix, origin.x, origin.y, size.x, size.y);
                                    gizmo->manipulate(currentGizmoOperation, currentGizmoAlignment, matrix, snapData, &boundingBox, currentGizmoAxis);
                                    if (gizmo->isUsing())
                                    {
                                        switch (currentGizmoOperation)
                                        {
                                        case UI::Gizmo::Operation::Translate:
                                            transformComponent.position = matrix.translation.xyz;
                                            break;

                                        case UI::Gizmo::Operation::Rotate:
                                            transformComponent.rotation = matrix.getRotation();
                                            break;

                                        case UI::Gizmo::Operation::Scale:
                                            transformComponent.scale = matrix.getScaling();
                                            break;

                                        case UI::Gizmo::Operation::Bounds:
                                            transformComponent.position = matrix.translation.xyz;
                                            transformComponent.scale = matrix.getScaling();
                                            break;
                                        };

                                        onModified(selectedEntity, typeid(Components::Transform));
                                    }
                                }
                            }
                        }
                    }
                }

                dock->EndTab();
            }

            bool createNamedEntity = true;
            std::string entityName;
            Plugin::Entity *selectedEntity = nullptr;
            bool showPopulationDock = true;
            void showPopulation(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                auto &style = ImGui::GetStyle();
                dock->SetNextLocation(UI::Dock::Location::Right);
                if (dock->BeginTab("Population", &showPopulationDock, 0, ImVec2(imGuiIo.DisplaySize.x * 0.3f, -1.0f)))
                {
                    ImGui::BulletText("Alignment ");
                    ImGui::SameLine();
                    auto width = (ImGui::GetContentRegionAvailWidth() - style.ItemSpacing.x) * 0.5f;
                    UI::RadioButton(ICON_FA_USER_O " Entity", &currentGizmoAlignment, UI::Gizmo::Alignment::Local, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(ICON_FA_GLOBE " World", &currentGizmoAlignment, UI::Gizmo::Alignment::World, ImVec2(width, 0.0f));

                    ImGui::BulletText("Operation ");
                    ImGui::SameLine();
                    width = (ImGui::GetContentRegionAvailWidth() - style.ItemSpacing.x * 3.0f) / 4.0f;
                    UI::RadioButton(ICON_FA_ARROWS " Move", &currentGizmoOperation, UI::Gizmo::Operation::Translate, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(ICON_FA_REPEAT " Rotate", &currentGizmoOperation, UI::Gizmo::Operation::Rotate, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(ICON_FA_SEARCH " Scale", &currentGizmoOperation, UI::Gizmo::Operation::Scale, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(ICON_FA_SEARCH " Bounds", &currentGizmoOperation, UI::Gizmo::Operation::Bounds, ImVec2(width, 0.0f));

                    ImGui::BulletText("Bounding Axis ");
                    ImGui::SameLine();
                    width = (ImGui::GetContentRegionAvailWidth() - style.ItemSpacing.x * 3.0f) / 4.0f;
                    UI::RadioButton(ICON_FA_SEARCH " Auto", &currentGizmoAxis, UI::Gizmo::LockAxis::Automatic, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(" X ", &currentGizmoAxis, UI::Gizmo::LockAxis::X, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(" Y ", &currentGizmoAxis, UI::Gizmo::LockAxis::Y, ImVec2(width, 0.0f));
                    ImGui::SameLine();
                    UI::RadioButton(" Z ", &currentGizmoAxis, UI::Gizmo::LockAxis::Z, ImVec2(width, 0.0f));

                    UI::CheckButton(ICON_FA_MAGNET " Snap", &useGizmoSnap);
                    ImGui::SameLine();
                    ImGui::PushItemWidth(-1.0f);
                    switch (currentGizmoOperation)
                    {
                    case UI::Gizmo::Operation::Translate:
                        ImGui::InputFloat3("##snapTranslation", gizmoSnapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        break;

                    case UI::Gizmo::Operation::Rotate:
                        ImGui::SliderFloat("##snapDegrees", &gizmoSnapRotation, 0.0f, 360.0f);
                        break;

                    case UI::Gizmo::Operation::Scale:
                        ImGui::InputFloat("##gizmoSnapScale", &gizmoSnapScale, (1.0f / 10.0f), 1.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        break;

                    case UI::Gizmo::Operation::Bounds:
                        ImGui::InputFloat3("##snapBounds", gizmoSnapBounds.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                        break;
                    };

                    ImGui::PopItemWidth();

                    std::set<Plugin::Entity *> deleteEntitySet;
                    auto &entityList = population->getEntityList();
                    auto entityCount = entityList.size();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.75f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    if (ImGui::Button(ICON_FA_USER_PLUS))
                    {
                        ImGui::OpenPopup("NewEntity");
                        createNamedEntity = true;
                        entityName.clear();
                    }

                    ImGui::PopStyleColor(3);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                    if (ImGui::BeginPopup("NewEntity"))
                    {
                        UI::TextFrame("Create Entity", ImVec2(ImGui::GetWindowContentRegionWidth(), 0.0f));
                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::Spacing();

                        if (ImGui::RadioButton("Named", createNamedEntity))
                        {
                            createNamedEntity = true;
                        }

                        ImGui::SameLine();
                        if (ImGui::RadioButton("Blank", !createNamedEntity))
                        {
                            createNamedEntity = false;
                        }

                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, createNamedEntity ? style.Colors[ImGuiCol_FrameBg] : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, createNamedEntity ? style.Colors[ImGuiCol_Text] : ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        UI::InputString("##name", entityName, (createNamedEntity ? 0 : ImGuiInputTextFlags_ReadOnly));
                        ImGui::PopStyleColor(2);

                        ImGui::Spacing();
                        if (ImGui::Button("Create", ImVec2(50.0f, 25.0f)))
                        {
                            std::vector<Plugin::Population::Component> componentList;
                            if (createNamedEntity && !entityName.empty())
                            {
                                componentList.push_back(std::make_pair("Name", entityName));
                            }

                            population->createEntity(componentList);
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(50.0f, 25.0f)))
                        {
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::PopStyleVar();
                    ImGui::SameLine();
                    UI::TextFrame(String::Format("Population: %v", entityCount).c_str(), ImVec2(ImGui::GetWindowContentRegionWidth(), 0.0f));
                    if (ImGui::BeginChildFrame(665, ImVec2(-1.0f, -1.0f)))
                    {
                        ImGuiListClipper clipper(entityCount);
                        while (clipper.Step())
                        {
                            for (auto entityIndex = clipper.DisplayStart; entityIndex < clipper.DisplayEnd; ++entityIndex)
                            {
                                auto entitySearch = std::begin(entityList);
                                std::advance(entitySearch, entityIndex);
                                auto entity = entitySearch->get();

                                std::string name;
                                if (entity->hasComponent<Components::Name>())
                                {
                                    name = entity->getComponent<Components::Name>().name;
                                }
                                else
                                {
                                    name = String::Format("entity_%v", entityIndex);
                                }

                                ImGui::PushID(entityIndex);
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                                if (ImGui::Button(ICON_FA_USER_TIMES))
                                {
                                    ImGui::OpenPopup("ConfirmEntityDelete");
                                }

                                ImGui::PopStyleColor(3);
                                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                                if (ImGui::BeginPopup("ConfirmEntityDelete"))
                                {
                                    ImGui::Text("Are you sure you want to remove this entitiy?");
                                    ImGui::Spacing();
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

                                ImGui::PopStyleVar();
                                ImGui::SameLine();
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.75f, 0.0f, 1.0f));
                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                                if (ImGui::Button(ICON_FA_PLUS_CIRCLE))
                                {
                                    selectedComponent = 0;
                                    ImGui::OpenPopup("AddComponent");
                                }

                                ImGui::PopStyleColor(3);
                                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                                if (ImGui::BeginPopup("AddComponent"))
                                {
                                    UI::TextFrame("Select Component Type", ImVec2(ImGui::GetWindowContentRegionWidth(), 0.0f));
                                    ImGui::Spacing();
                                    ImGui::Spacing();
                                    ImGui::Spacing();

                                    const auto &componentMap = population->getComponentMap();
                                    auto componentCount = componentMap.size();
                                    if (ImGui::ListBoxHeader("##Components", componentCount, 10))
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

                                ImGui::PopStyleVar();
                                ImGui::PopID();
                                ImGui::SameLine();
                                ImGui::SetNextTreeNodeOpen(selectedEntity == entity);
                                if (ImGui::TreeNodeEx(name.c_str(), ImGuiTreeNodeFlags_Framed))
                                {
                                    selectedEntity = entity;
                                    auto editorEntity = dynamic_cast<Edit::Entity *>(entity);
                                    if (editorEntity)
                                    {
                                        std::set<std::type_index> deleteComponentSet;
                                        const auto &entityComponentMap = editorEntity->getComponentMap();
                                        for (auto &componentSearch : entityComponentMap)
                                        {
                                            Edit::Component *component = population->getComponent(componentSearch.first);
                                            Plugin::Component::Data *componentData = componentSearch.second.get();
                                            if (component && componentData)
                                            {
                                                ImGui::PushID(component->getIdentifier().hash_code());
                                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
                                                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
                                                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                                                if (ImGui::Button(ICON_FA_MINUS_CIRCLE))
                                                {
                                                    ImGui::OpenPopup("ConfirmComponentDelete");
                                                }

                                                ImGui::PopStyleColor(3);
                                                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                                                if (ImGui::BeginPopup("ConfirmComponentDelete"))
                                                {
                                                    ImGui::Text("Are you sure you want to remove this component?");
                                                    ImGui::Spacing();
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

                                                ImGui::PopStyleVar();
                                                ImGui::PopID();
                                                ImGui::SameLine();
                                                if (ImGui::TreeNodeEx(component->getName().c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                                                {
                                                    if (component->onUserInterface(ImGui::GetCurrentContext(), entity, componentData))
                                                    {
                                                        onModified(entity, componentSearch.first);
                                                    }

                                                    ImGui::TreePop();
                                                }
                                            }
                                        }

                                        for (auto &component : deleteComponentSet)
                                        {
                                            population->removeComponent(entity, component);
                                        }
                                    }

                                    ImGui::TreePop();
                                }
                                else if (selectedEntity == entity)
                                {
                                    selectedEntity = nullptr;
                                }
                            }
                        };

                    }

                    ImGui::EndChildFrame();
                    for (auto &entity : deleteEntitySet)
                    {
                        if (selectedEntity == entity)
                        {
                            selectedEntity = nullptr;
                        }

                        population->killEntity(entity);
                    }
                }

                dock->EndTab();
            }

            void onShowUserInterface(ImGuiContext * const guiContext)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                bool mainMenuShowing = (ImGui::FindWindowByName("##MainMenuBar") ? true : false);
                if (mainMenuShowing)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("Edit"))
                    {
                        bool editorEnabled = core->getOption("editor", "active").convert(false);
                        if (ImGui::MenuItem("Enable", "CTRL+E", &editorEnabled))
                        {
                            core->setOption("editor", "active", editorEnabled);
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();
                }

                bool editorActive = core->getOption("editor", "active").convert(false);
                if (!editorActive)
                {
                    return;
                }

                auto editorSize = imGuiIo.DisplaySize;
                auto editorPosition = ImVec2(0.0f, 0.0f);
                if (mainMenuShowing)
                {
                    auto &style = ImGui::GetStyle();
                    editorSize.y -= ImGui::GetItemsLineHeightWithSpacing() - style.ItemSpacing.y;
                    editorPosition.y += ImGui::GetItemsLineHeightWithSpacing() - style.ItemSpacing.y;
                }

                ImGui::SetNextWindowSize(editorSize);
                ImGui::SetNextWindowPos(editorPosition);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                auto oldWindowPadding = UI::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                if (ImGui::Begin("Editor", nullptr, editorSize, 1.0f, ImGuiWindowFlags_ShowBorders | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, oldWindowPadding);
                    dock->Begin("##Editor", (UI::GetWindowContentRegionSize()), true, ImVec2(10.0f, 10.0f));

                    showScene();
                    showPopulation();

                    dock->End();
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
                    Math::Float4x4 viewMatrix(Math::Float4x4::MakePitchRotation(lookingAngle) * Math::Float4x4::MakeYawRotation(headingAngle));
                    position += (viewMatrix.rz.xyz * (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f) * frameTime);
                    position += (viewMatrix.rx.xyz * (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f) * frameTime);
                    viewMatrix.translation.xyz = position;
                    viewMatrix.invert();

                    auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f));

                    renderer->queueCamera(viewMatrix, projectionMatrix, 0.1f, 200.0f, cameraTarget, "editor");
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
