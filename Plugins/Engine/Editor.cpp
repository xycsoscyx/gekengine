#include "GEK/Math/Common.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/API/Component.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Entity.hpp"
#include "GEK/API/Processor.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/API/Visualizer.hpp"
#include "GEK/Engine/Resources.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Components/Name.hpp"
#include "GEK/Model/Base.hpp"
#include <set>

namespace Gek
{
    namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , virtual public Plugin::Processor
            , virtual public Edit::Events
        {
            enum AxisSelection
            {
                ALL,
                X,
                Y,
                Z,
            };

        private:
            Plugin::Core *core = nullptr;
            Edit::Population *population = nullptr;
            Engine::Resources *resources = nullptr;
            Plugin::Visualizer *renderer = nullptr;
            Gek::Processor::Model *modelProcessor = nullptr;

            float headingAngle = 0.0f;
            float lookingAngle = 0.0f;
            Math::Float3 position = Math::Float3::Zero;
            bool moveForward = false;
            bool moveBackward = false;
            bool strafeLeft = false;
            bool strafeRight = false;

            int selectedComponent = 0;

            AxisSelection currentAxis = AxisSelection::ALL;
            ImGuizmo::MODE currentGizmoAlignment = ImGuizmo::MODE::LOCAL;
            ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
            bool useGizmoSnap = true;
            Math::Float3 gizmoSnapPosition = Math::Float3::One;
            float gizmoSnapRotation = 10.0f;
            float gizmoSnapScale = 1.0f;
            Math::Float3 gizmoSnapBounds = Math::Float3::One;

            ResourceHandle cameraTarget;
            ImVec2 cameraSize;

            bool createBlankEntity = true;
            bool createNamedEntity = true;
            bool includeTransform = true;
            std::string entityName;
            Plugin::Entity *selectedEntity = nullptr;
            bool showPopulationDock = true;
            bool showEntityDock = true;

            bool sceneModified = false;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Engine::Core *>(core)->getFullPopulation())
                , resources(dynamic_cast<Engine::Core *>(core)->getFullResources())
                , renderer(core->getVisualizer())
            {
                assert(population);
                assert(core);

                core->setOption("editor", "active", false);

                core->onInitialized.connect(this, &Editor::onInitialized);
                core->canShutdown.connect(this, &Editor::canShutdown);
                core->onShutdown.connect(this, &Editor::onShutdown);
				population->onReset.connect(this, &Editor::onReset);
				population->onAction.connect(this, &Editor::onAction);
                population->onUpdate[90].connect(this, &Editor::onUpdate);
                renderer->onShowUserInterface.connect(this, &Editor::onShowUserInterface);
            }

            void modify(Plugin::Entity* entity, Hash type)
            {
                onModified(entity, type);
                sceneModified = true;
            }

            // Edit::Events
            bool isModified(void)
            {
                return sceneModified;
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

            void canShutdown(bool &shutdown)
            {
                shutdown = !sceneModified;
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
                    float distance = plane.getDistance(orientedBox.matrix.translation());
                    float radiusX = std::abs(orientedBox.matrix.r.x.xyz().dot(plane.vector.xyz()) * orientedBox.halfsize.x);
                    float radiusY = std::abs(orientedBox.matrix.r.y.xyz().dot(plane.vector.xyz()) * orientedBox.halfsize.y);
                    float radiusZ = std::abs(orientedBox.matrix.r.z.xyz().dot(plane.vector.xyz()) * orientedBox.halfsize.z);
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

                    Render::Texture::Description description;
                    description.name = "editorTarget";
                    description.width = cameraSize.x;
                    description.height = cameraSize.y;
                    description.flags = Render::Texture::Flags::RenderTarget | Render::Texture::Flags::Resource;
                    description.format = Render::Format::R11G11B10_FLOAT;
                    cameraTarget = core->getResources()->createTexture(description, Plugin::Resources::Flags::LoadImmediately);

                    auto cameraBuffer = resources->getResource(cameraTarget);
                    auto cameraTexture = (cameraBuffer ? dynamic_cast<Render::Texture *>(cameraBuffer) : nullptr);
                    if (cameraTexture)
                    {
                        ImGui::Image(reinterpret_cast<ImTextureID>(cameraTexture), cameraSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f), ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

                        auto projectionMatrix(Math::Float4x4::MakePerspective(Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f));
                        Math::Float4x4 viewMatrix(Math::Float4x4::MakePitchRotation(lookingAngle) * Math::Float4x4::MakeYawRotation(headingAngle));
                        viewMatrix.translation() = position;
                        viewMatrix.invert();

                        const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
                        ImGui::Begin("gizmo", NULL, flags);
                        auto gizmoDrawList = ImGui::GetWindowDrawList();
                        ImGui::End();

                        //gizmoDrawList->AddCallback()
                        ImGuizmo::Enable(true);
                        ImGuizmo::BeginFrame();

                        auto size = ImGui::GetItemRectSize();
                        auto origin = ImGui::GetItemRectMin();
                        
                        ImGuizmo::SetDrawlist();
                        ImGuizmo::SetRect(origin.x, origin.y, size.x, size.y);

                        //ImGuizmo::DrawGrid(viewMatrix.data, projectionMatrix.data, Math::Float4x4::Identity.data, 25.0f);

                        auto& registry = population->getRegistry();
                        for (auto& entity : registry)
                        {
                            if (entity->hasComponent<Components::Transform>())
                            {
                                auto& transformComponent = entity->getComponent<Components::Transform>();

                                bool showCube = true;
                                if (entity->hasComponent<Components::Model>())
                                {
                                    auto& modelComponent = entity->getComponent<Components::Model>();
                                    showCube = modelComponent.name.empty();
                                }

                                if (showCube)
                                {
                                    auto matrix = transformComponent.getScaledMatrix();
                                    ImGuizmo::DrawCubes(viewMatrix.data, projectionMatrix.data, matrix.data, 1);
                                }
                            }
                        }

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
                                    case ImGuizmo::OPERATION::TRANSLATE:
                                        snapData = gizmoSnapPosition.data;
                                        break;

                                    case ImGuizmo::OPERATION::ROTATE:
                                        snapData = &gizmoSnapRotation;
                                        break;

                                    case ImGuizmo::OPERATION::SCALE:
                                        snapData = &gizmoSnapScale;
                                        break;

                                    case ImGuizmo::OPERATION::BOUNDS:
                                        snapData = gizmoSnapBounds.data;
                                        break;
                                    };
                                }

                                Shapes::AlignedBox boundingBox(1.0f);
                                if (selectedEntity->hasComponent<Components::Model>())
                                {
                                    auto &modelComponent = selectedEntity->getComponent<Components::Model>();
                                    if (!modelComponent.name.empty())
                                    {
                                        boundingBox = modelProcessor->getBoundingBox(modelComponent.name);
                                    }
                                }

                                auto projectedMatrix = Shapes::Frustum(viewMatrix * projectionMatrix);
                                auto orientedBox = Shapes::OrientedBox(matrix, boundingBox);
                                if (isObjectInFrustum(projectedMatrix, orientedBox))
                                {
                                    static std::map<ImGuizmo::OPERATION, std::map<AxisSelection, ImGuizmo::OPERATION>> lockedOperations =
                                    {
                                        { ImGuizmo::OPERATION::TRANSLATE, {
                                            { AxisSelection::ALL, ImGuizmo::OPERATION::TRANSLATE },
                                            { AxisSelection::X, ImGuizmo::OPERATION::TRANSLATE_X },
                                            { AxisSelection::Y, ImGuizmo::OPERATION::TRANSLATE_Y },
                                            { AxisSelection::Z, ImGuizmo::OPERATION::TRANSLATE_Z }
                                        } },
                                        { ImGuizmo::OPERATION::ROTATE, {
                                            { AxisSelection::ALL, ImGuizmo::OPERATION::ROTATE },
                                            { AxisSelection::X, ImGuizmo::OPERATION::ROTATE_X },
                                            { AxisSelection::Y, ImGuizmo::OPERATION::ROTATE_Y },
                                            { AxisSelection::Z, ImGuizmo::OPERATION::ROTATE_Z }
                                        } },
                                        { ImGuizmo::OPERATION::SCALE, {
                                            { AxisSelection::ALL, ImGuizmo::OPERATION::SCALE },
                                            { AxisSelection::X, ImGuizmo::OPERATION::SCALE_X },
                                            { AxisSelection::Y, ImGuizmo::OPERATION::SCALE_Y },
                                            { AxisSelection::Z, ImGuizmo::OPERATION::SCALE_Z }
                                        } },
                                        { ImGuizmo::OPERATION::BOUNDS, {
                                            { AxisSelection::ALL, ImGuizmo::OPERATION::BOUNDS },
                                            { AxisSelection::X, ImGuizmo::OPERATION::BOUNDS },
                                            { AxisSelection::Y, ImGuizmo::OPERATION::BOUNDS },
                                            { AxisSelection::Z, ImGuizmo::OPERATION::BOUNDS }
                                        } },
                                    };

                                    ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, lockedOperations[currentGizmoOperation][currentAxis], currentGizmoAlignment, matrix.data, nullptr, snapData, boundingBox.minimum.data);
                                    if (ImGuizmo::IsUsing())
                                    {
                                        switch (currentGizmoOperation)
                                        {
                                        case ImGuizmo::OPERATION::TRANSLATE:
                                            transformComponent.position = matrix.translation();
                                            break;

                                        case ImGuizmo::OPERATION::ROTATE:
                                            transformComponent.rotation = matrix.getRotation();
                                            break;

                                        case ImGuizmo::OPERATION::SCALE:
                                            transformComponent.scale = matrix.getScaling();
                                            break;

                                        case ImGuizmo::OPERATION::BOUNDS:
                                            transformComponent.position = matrix.translation();
                                            transformComponent.scale = matrix.getScaling();
                                            break;
                                        };

										modify(selectedEntity, Components::Transform::GetIdentifier());
                                    }
                                }
                            }
                        }

                        ImGuizmo::ViewManipulate(viewMatrix.data, 5.0f, ImVec2(origin.x + size.x - 75.0f, origin.y), ImVec2(75.0f, 75.0f), 0x10101010);
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
                        createBlankEntity = true;
                        createNamedEntity = false;
                        includeTransform = true;
                        entityName.clear();
                    }

                    ImGui::PopStyleColor(3);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                    if (ImGui::BeginPopup("NewEntity"))
                    {
                        UI::TextFrame("Create Entity", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
                        ImGui::Spacing();
                        ImGui::Spacing();
                        ImGui::Spacing();

                        if (ImGui::RadioButton("Blank", createBlankEntity))
                        {
                            createBlankEntity = true;
                            createNamedEntity = false;
                        }

                        ImGui::SameLine();
                        if (ImGui::RadioButton("Named", createNamedEntity))
                        {
                            createBlankEntity = false;
                            createNamedEntity = true;
                        }

                        ImGui::Checkbox("Include Transform", &includeTransform);

                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, createNamedEntity ? style.Colors[ImGuiCol_FrameBg] : ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_Text, createNamedEntity ? style.Colors[ImGuiCol_Text] : ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                        if (createNamedEntity)
                        {
                            UI::InputString("##name", entityName);
                        }
                        else
                        {
                            std::string unnamed("<unnamed>");
                            UI::InputString("##blank", unnamed, ImGuiInputTextFlags_ReadOnly);
                        }

                        ImGui::PopStyleColor(2);

                        ImGui::Spacing();
                        if (ImGui::Button("Create"))
                        {
                            Plugin::Population::EntityDefinition definition;
                            if (createNamedEntity && !entityName.empty())
                            {
                                definition["Name"] = entityName;
                            }

                            if (includeTransform)
                            {
                                definition["Transform"]["position"] = { 0.0f, 0.0f, 0.0f };
                                definition["Transform"]["rotation"] = { 0.0f, 0.0f, 0.0f, 1.0f };
                                definition["Transform"]["scale"] = { 1.0f, 1.0f, 1.0f };

                            }

                            selectedEntity = population->createEntity(definition);
                            ImGui::CloseCurrentPopup();
                            sceneModified = true;
                        }

                        ImGui::SameLine();
                        if (ImGui::Button("Cancel"))
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
                                if (ImGui::Button("Yes"))
                                {
                                    sceneModified = true;
                                    deleteEntitySet.insert(entity.get());
                                    ImGui::CloseCurrentPopup();
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("No"))
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

            void showEntity(void)
            {
                auto& imGuiIo = ImGui::GetIO();
                auto& style = ImGui::GetStyle();
                if (ImGui::Begin("Entity", &showEntityDock))//, ImVec2(imGuiIo.DisplaySize.x * 0.3f, -1.0f)))
                {
                    if (selectedEntity)
                    {
                        ImGui::BulletText("Alignment ");
                        ImGui::SameLine();
                        auto width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x) * 0.5f;
                        UI::RadioButton(std::format("{} Entity", (const char*)ICON_FA_USER_O), &currentGizmoAlignment, ImGuizmo::MODE::LOCAL, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} World", (const char*)ICON_FA_GLOBE), &currentGizmoAlignment, ImGuizmo::MODE::WORLD, ImVec2(width, 0.0f));

                        ImGui::BulletText("Operation ");
                        ImGui::SameLine();
                        width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f;
                        UI::RadioButton(std::format("{} Move", (const char*)ICON_FA_ARROWS), &currentGizmoOperation, ImGuizmo::OPERATION::TRANSLATE, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Rotate", (const char*)ICON_FA_REPEAT), &currentGizmoOperation, ImGuizmo::OPERATION::ROTATE, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Scale", (const char*)ICON_FA_SEARCH), &currentGizmoOperation, ImGuizmo::OPERATION::SCALE, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(std::format("{} Bounds", (const char*)ICON_FA_SEARCH), &currentGizmoOperation, ImGuizmo::OPERATION::BOUNDS, ImVec2(width, 0.0f));

                        ImGui::BulletText("Axis ");
                        ImGui::SameLine();
                        width = (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x * 3.0f) / 4.0f;
                        UI::RadioButton(std::format("{} All", (const char*)ICON_FA_SEARCH), &currentAxis, AxisSelection::ALL, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" X ", &currentAxis, AxisSelection::X, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" Y ", &currentAxis, AxisSelection::Y, ImVec2(width, 0.0f));
                        ImGui::SameLine();
                        UI::RadioButton(" Z ", &currentAxis, AxisSelection::Z, ImVec2(width, 0.0f));

                        UI::CheckButton(std::format("{} Snap", (const char*)ICON_FA_MAGNET), &useGizmoSnap);
                        ImGui::SameLine();
                        ImGui::PushItemWidth(-1.0f);
                        switch (currentGizmoOperation)
                        {
                        case ImGuizmo::OPERATION::TRANSLATE:
                            ImGui::InputFloat3("##snapTranslation", gizmoSnapPosition.data, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            break;

                        case ImGuizmo::OPERATION::ROTATE:
                            ImGui::SliderFloat("##snapDegrees", &gizmoSnapRotation, 0.0f, 360.0f);
                            break;

                        case ImGuizmo::OPERATION::SCALE:
                            ImGui::InputFloat("##gizmoSnapScale", &gizmoSnapScale, (1.0f / 10.0f), 1.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                            break;

                        case ImGuizmo::OPERATION::BOUNDS:
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
                                UI::TextFrame("Select Component Type", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
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
                                            auto componentDefintion = std::make_pair(componentName, JSON::Object());
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
                                                if (ImGui::Button("Yes"))
                                                {
                                                    ImGui::CloseCurrentPopup();
                                                    deleteComponentSet.insert(component->getIdentifier());
                                                }

                                                ImGui::SameLine();
                                                if (ImGui::Button("No"))
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
                                                    modify(entity, componentSearch.first);
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
                        bool editorEnabled = core->getOption("editor", "active", false);
                        if (ImGui::MenuItem("Show Editor", nullptr, &editorEnabled))
                        {
                            core->setOption("editor", "active", editorEnabled);
                        }

                        ImGui::EndMenu();
                    }

                    ImGui::PopStyleVar(2);
                    ImGui::EndMainMenuBar();
                }

                bool editorActive = core->getOption("editor", "active", false);
                if (!editorActive)
                {
                    return;
                }

                auto dockspaceID = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

                ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
                showScene();

                ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
                showEntity();

                ImGui::SetNextWindowDockID(dockspaceID, ImGuiCond_FirstUseEver);
                showPopulation();
            }

            // Plugin::Population Slots
            void onReset(void)
			{
                sceneModified = false;
				selectedEntity = nullptr;
			}

            void onAction(Plugin::Population::Action const &action)
            {
                bool editorActive = core->getOption("editor", "active", false);
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
                bool editorActive = core->getOption("editor", "active", false);
                if (editorActive)
                {
                    Math::Float4x4 viewMatrix(Math::Float4x4::MakePitchRotation(lookingAngle) * Math::Float4x4::MakeYawRotation(headingAngle));
                    position += (viewMatrix.r.z.xyz() * (((moveForward ? 1.0f : 0.0f) + (moveBackward ? -1.0f : 0.0f)) * 5.0f) * frameTime);
                    position += (viewMatrix.r.x.xyz() * (((strafeLeft ? -1.0f : 0.0f) + (strafeRight ? 1.0f : 0.0f)) * 5.0f) * frameTime);
                    viewMatrix.translation() = position;
                    viewMatrix.invert();

                    renderer->queueCamera(viewMatrix, Math::DegreesToRadians(90.0f), (cameraSize.x / cameraSize.y), 0.1f, 200.0f, "Editor Camera"s, cameraTarget, "editor");
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
