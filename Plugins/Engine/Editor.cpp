#include "GEK/Math/Common.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/GUI/Gizmo.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/API/Renderer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Name.hpp"
#include "GEK/Model/Base.hpp"
#include <concurrent_vector.h>
#include <imgui_internal.h>
#include <ppl.h>
#include <set>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , virtual public Plugin::Processor
            , virtual public Edit::Events
        {
        private:
            Plugin::Core *core = nullptr;
            Edit::Population *population = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Renderer *renderer = nullptr;
            Gek::Processor::Model *modelProcessor = nullptr;

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

            bool createNamedEntity = true;
            std::string entityName;
            Plugin::Entity *selectedEntity = nullptr;
            bool showPopulationDock = true;
            bool showPropertiesDock = true;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Engine::Core *>(core)->getFullPopulation())
                , resources(dynamic_cast<Engine::Core *>(core)->getFullResources())
                , renderer(core->getRenderer())
            {
                assert(population);
                assert(core);

                core->setOption("editor", "active", false);

                gizmo = std::make_unique<UI::Gizmo::WorkSpace>();

                core->onInitialized.connect(this, &Editor::onInitialized);
                core->onShutdown.connect(this, &Editor::onShutdown);
				population->onReset.connect(this, &Editor::onReset);
				population->onAction.connect(this, &Editor::onAction);
                population->onUpdate[90].connect(this, &Editor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Editor::onShowUserInterface);
            }

            // Plugin::Core
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

            void onShutdown(void)
            {
                renderer->onShowUserInterface.disconnect(this, &Editor::onShowUserInterface);
                population->onAction.disconnect(this, &Editor::onAction);
                population->onUpdate[90].disconnect(this, &Editor::onUpdate);
				population->onReset.disconnect(this, &Editor::onReset);
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
				if (ImGui::Begin("Scene", &showSceneDock, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
                {
                    cameraSize = UI::GetWindowContentRegionSize();

                    Video::Texture::Description description;
                    description.width = cameraSize.x;
                    description.height = cameraSize.y;
                    description.flags = Video::Texture::Flags::RenderTarget | Video::Texture::Flags::Resource;
                    description.format = Video::Format::R11G11B10_FLOAT;
                    cameraTarget = core->getResources()->createTexture("editorTarget", description, Plugin::Resources::Flags::LoadImmediately);

                    auto cameraBuffer = resources->getResource(cameraTarget);
                    auto cameraTexture = (cameraBuffer ? dynamic_cast<Video::Texture *>(cameraBuffer) : nullptr);
                    if (cameraTexture)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(cameraTexture), cameraSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
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

                                auto projectedMatrix = Shapes::Frustum(viewMatrix * projectionMatrix);
                                auto orientedBox = Shapes::OrientedBox(matrix, boundingBox);
                                if (isObjectInFrustum(projectedMatrix, orientedBox))
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

										onModified(selectedEntity, Components::Transform::GetIdentifier());
                                    }
                                }
                            }
                        }
                    }
                }

                ImGui::End();
            }

            void showPopulation(void)
            {
                auto &imGuiIo = ImGui::GetIO();
                auto &style = ImGui::GetStyle();
                if (ImGui::Begin("Population", &showPopulationDock))//, ImVec2(imGuiIo.DisplaySize.x * 0.3f, -1.0f)))
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.75f, 0.0f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                    if (ImGui::Button((const char *)ICON_FA_USER_PLUS))
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
                            Plugin::Population::EntityDefinition definition;
                            if (createNamedEntity && !entityName.empty())
                            {
                                definition["Name"] = JSON(entityName);
                            }

                            population->createEntity(definition);
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

                    std::set<Plugin::Entity*> deleteEntitySet;
                    auto& registry = population->getRegistry();
                    auto entityCount = registry.size();

                    if (ImGui::BeginListBox("##Population", ImVec2(-FLT_MIN, -FLT_MIN)))
                    {
                        for (auto& entity : registry)
                        {
                            Edit::Entity* editEntity = reinterpret_cast<Edit::Entity*>(entity.get());

                            std::string name;
                            if (entity->hasComponent<Components::Name>())
                            {
                                name = entity->getComponent<Components::Name>().name;
                            }
                            else
                            {
                                name = "<unnamed>";
                            }

                            name = std::format("{}, {} components", name, editEntity->getComponents().size());

                            ImGui::PushID(entity.get());
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                            if (ImGui::Button((const char*)ICON_FA_USER_TIMES))
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
                                    deleteEntitySet.insert(entity.get());
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
                            bool entitySelected = (selectedEntity == entity.get());
                            if (ImGui::Selectable(name.data(), &entitySelected))
                            {
                                selectedEntity = entity.get();
                            }

                            ImGui::PopID();
                        }

                        ImGui::EndListBox();
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

                ImGui::End();
            }

            void showProperties(void)
            {
                auto& imGuiIo = ImGui::GetIO();
                auto& style = ImGui::GetStyle();
                if (ImGui::Begin("Properties", &showPropertiesDock))//, ImVec2(imGuiIo.DisplaySize.x * 0.3f, -1.0f)))
                {
                    if (selectedEntity)
                    {
                        ImGui::BulletText("Alignment ");
                        ImGui::SameLine();
                        auto width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.5f;
                        UI::RadioButton(std::format("{} Entity", (const char*)ICON_FA_USER_O), &currentGizmoAlignment, UI::Gizmo::Alignment::Local, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} World", (const char*)ICON_FA_GLOBE), &currentGizmoAlignment, UI::Gizmo::Alignment::World, ImVec2(width, 0.0f));

                        ImGui::BulletText("Operation ");
                        ImGui::SameLine();
                        width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f;
                        UI::RadioButton(std::format("{} Move", (const char*)ICON_FA_ARROWS), &currentGizmoOperation, UI::Gizmo::Operation::Translate, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Rotate", (const char*)ICON_FA_REPEAT), &currentGizmoOperation, UI::Gizmo::Operation::Rotate, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Scale", (const char*)ICON_FA_SEARCH), &currentGizmoOperation, UI::Gizmo::Operation::Scale, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Bounds", (const char*)ICON_FA_SEARCH), &currentGizmoOperation, UI::Gizmo::Operation::Bounds, ImVec2(width, 0.0f));

                        ImGui::BulletText("Bounding Axis ");
                        ImGui::SameLine();
                        width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f;
                        UI::RadioButton(std::format("{} Auto", (const char*)ICON_FA_SEARCH), &currentGizmoAxis, UI::Gizmo::LockAxis::Automatic, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" X ", &currentGizmoAxis, UI::Gizmo::LockAxis::X, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" Y ", &currentGizmoAxis, UI::Gizmo::LockAxis::Y, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" Z ", &currentGizmoAxis, UI::Gizmo::LockAxis::Z, ImVec2(width, 0.0f));

                        UI::CheckButton(std::format("{} Snap", (const char*)ICON_FA_MAGNET), &useGizmoSnap);
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-1.0f);
                        switch (currentGizmoOperation)
                        {
                        case UI::Gizmo::Operation::Translate:
                            ImGui::InputFloat3("##snapTranslation", gizmoSnapPosition.data, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            break;

                        case UI::Gizmo::Operation::Rotate:
                            ImGui::SliderFloat("##snapDegrees", &gizmoSnapRotation, 0.0f, 360.0f);
                            break;

                        case UI::Gizmo::Operation::Scale:
                            ImGui::InputFloat("##gizmoSnapScale", &gizmoSnapScale, (1.0f / 10.0f), 1.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            break;

                        case UI::Gizmo::Operation::Bounds:
                            ImGui::InputFloat3("##snapBounds", gizmoSnapBounds.data, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            break;
                        };

                        ImGui::PopItemWidth();
                        if (ImGui::BeginChildFrame(665, ImVec2(-1.0f, -1.0f)))
                        {
                            auto entity = selectedEntity;
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.75f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                            if (ImGui::Button((const char*)ICON_FA_PLUS_CIRCLE))
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

                                auto const& availableComponents = population->getAvailableComponents();
                                auto componentCount = availableComponents.size();
                                if (ImGui::BeginListBox("##Components"))
                                {
                                    for (auto &component : availableComponents)
                                    {
                                        std::string componentName(component.second->getName());

                                        ImGui::PushID(component.second->getIdentifier());

                                        bool componentSelected = false;
                                        if (ImGui::Selectable(componentName.data(), &componentSelected))
                                        {
                                            auto componentDefintion = std::make_pair(componentName, JSON::EmptyObject);
                                            population->addComponent(entity, componentDefintion);
                                            ImGui::CloseCurrentPopup();
                                        }

                                        ImGui::PopID();
                                    }

                                    ImGui::EndListBox();
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::PopStyleVar();
                            ImGui::SameLine();
                            ImGui::SetNextItemOpen(selectedEntity == entity);
                            if (ImGui::TreeNodeEx("##selected", ImGuiTreeNodeFlags_Framed))
                            {
                                selectedEntity = dynamic_cast<Edit::Entity*>(entity);
                                auto editEntity = dynamic_cast<Edit::Entity*>(selectedEntity);
                                if (editEntity)
                                {
                                    std::set<Hash> deleteComponentSet;
                                    auto const& entityComponents = editEntity->getComponents();
                                    for (auto& componentSearch : entityComponents)
                                    {
                                        Edit::Component* component = population->getComponent(componentSearch.first);
                                        Plugin::Component::Data* componentDefintion = componentSearch.second.get();
                                        if (component && componentDefintion)
                                        {
                                            ImGui::PushID(component->getIdentifier());
                                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
                                            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.0f, 0.0f, 1.0f));
                                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                                            if (ImGui::Button((const char*)ICON_FA_MINUS_CIRCLE))
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
                                            if (ImGui::TreeNodeEx(component->getName().data(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
                                            {
                                                if (component->onUserInterface(ImGui::GetCurrentContext(), entity, componentDefintion))
                                                {
                                                    onModified(entity, componentSearch.first);
                                                }

                                                ImGui::TreePop();
                                            }
                                        }
                                    }

                                    for (auto& component : deleteComponentSet)
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

                        ImGui::EndChildFrame();
                    }
                }

                ImGui::End();
            }

            void onShowUserInterface(void)
            {
                ImGuiIO &imGuiIo = ImGui::GetIO();
                auto mainMenu = ImGui::FindWindowByName("##MainMenuBar");
                auto mainMenuShowing = (mainMenu ? mainMenu->Active : false);
                if (mainMenuShowing)
                {
                    ImGui::BeginMainMenuBar();
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(5.0f, 10.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 10.0f));
                    if (ImGui::BeginMenu("Edit"))
                    {
                        bool editorEnabled = core->getOption("editor", "active").convert(false);
                        if (ImGui::MenuItem("Show Editor", nullptr, &editorEnabled))
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

                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
                showScene();
                showPopulation();
                showProperties();
            }

            // Plugin::Population Slots
			void onReset(void)
			{
				selectedEntity = nullptr;
			}

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

                    renderer->queueCamera(viewMatrix, Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f, "Editor Camera"s, cameraTarget, "editor");
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
