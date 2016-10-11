﻿#include "GEK\Math\Common.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\Editor.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Component.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
    namespace Private
    {
        void EditorCamera::save(Xml::Leaf &componentData) const
        {
        }

        void EditorCamera::load(const Xml::Leaf &componentData)
        {
        }
    }; // namespace Private

    namespace Implementation
    {
        GEK_CONTEXT_USER(EditorCamera)
            , public Plugin::ComponentMixin<Private::EditorCamera, Edit::Component>
        {
        public:
            EditorCamera(Context *context)
                : ContextRegistration(context)
            {
            }

            // Edit::Component
            void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
            {
                auto &editorCamera = *dynamic_cast<Private::EditorCamera *>(data);
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"editor_camera";
            }
        };

        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
            , public Plugin::Processor
            , public Plugin::CoreListener
        {
        private:
            Plugin::Core *core;
            Edit::Population *population;
            Plugin::Entity *camera = nullptr;

            uint32_t selectedEntity = 0;
            uint32_t selectedComponent = 0;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , core(core)
                , population(dynamic_cast<Edit::Population *>(core->getPopulation()))
            {
                GEK_REQUIRE(population);
                GEK_REQUIRE(core);
                core->addListener(this);
                population->addListener(this);
                population->addStep(this, -1);
            }

            ~Editor(void)
            {
                population->removeStep(this);
                population->removeListener(this);
                core->removeListener(this);
            }

            // Plugin::CoreListener
            void onAction(const wchar_t *actionName, const Plugin::ActionParameter &parameter)
            {
                if (camera)
                {
                    auto &editorCamera = camera->getComponent<Private::EditorCamera>();
                    if (_wcsicmp(actionName, L"turn") == 0)
                    {
                        editorCamera.headingAngle += (parameter.value * 0.01f);
                    }
                    else if (_wcsicmp(actionName, L"move_forward") == 0)
                    {
                        editorCamera.moveForward = parameter.state;
                    }
                    else if (_wcsicmp(actionName, L"move_backward") == 0)
                    {
                        editorCamera.moveBackward = parameter.state;
                    }
                    else if (_wcsicmp(actionName, L"strafe_left") == 0)
                    {
                        editorCamera.strafeLeft = parameter.state;
                    }
                    else if (_wcsicmp(actionName, L"strafe_right") == 0)
                    {
                        editorCamera.strafeRight = parameter.state;
                    }
                    else if (_wcsicmp(actionName, L"crouch") == 0)
                    {
                        editorCamera.crouching = parameter.state;
                    }
                }
            }

            // Plugin::PopulationListener
            void onLoadBegin(const String &populationName)
            {
                camera = nullptr;

                selectedEntity = 0;
                selectedComponent = 0;

                std::vector<Xml::Leaf> editorComponentList(2);
                editorComponentList[0].type = L"transform";
                editorComponentList[1].type = L"editor_camera";
                camera = population->createEntity(L"editor_camera", editorComponentList);
            }

            void onLoadSucceeded(const String &populationName)
            {
            }

            void onLoadFailed(const String &populationName)
            {
            }

            // Plugin::PopulationStep
            void onUpdate(uint32_t order, State state)
            {
                auto changeConfiguration = core->changeConfiguration();
                Xml::Node &configuration = *changeConfiguration;
                bool showSelectionMenu = configuration.getChild(L"editor").getAttribute(L"enabled", L"false");
                if (showSelectionMenu)
                {
                    ImGui::Begin("Level Editor", &showSelectionMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                    ImGui::Dummy(ImVec2(350, 0));

                    if (state == State::Loading)
                    {
                        ImGui::TextDisabled("Loading...");
                    }
                    else
                    {
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
                                    auto entitySearch = entityMap.begin();
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

                            auto entitySearch = entityMap.begin();
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
                                                auto componentSearch = componentMap.begin();
                                                std::advance(componentSearch, componentIndex);
                                                if (ImGui::Selectable((componentSearch->first.name() + 7), (selectedComponent == componentIndex)))
                                                {
                                                    Xml::Leaf componentData;
                                                    componentData.type = componentSearch->second->getName();
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
                                                auto entityComponentSearch = entityComponentMap.begin();
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

                                    auto entityComponentSearch = entityComponentMap.begin();
                                    std::advance(entityComponentSearch, selectedComponent);
                                    if (entityComponentSearch != entityComponentMap.end())
                                    {
                                        Edit::Component *component = population->getComponent(entityComponentSearch->first);
                                        Plugin::Component::Data *componentData = entityComponentSearch->second.get();
                                        if (component && componentData)
                                        {
                                            ImGui::Separator();

                                            auto cameraMatrix(camera->getComponent<Components::Transform>().getMatrix());
                                            auto viewMatrix(cameraMatrix.getInverse());

                                            const float width = 800.0f;
                                            const float height = 600.0f;
                                            auto projectionMatrix(Math::Float4x4::createPerspective(Math::convertDegreesToRadians(90.0f), (width / height), 0.1f, 200.0f));
                                            component->showEditor(ImGui::GetCurrentContext(), viewMatrix, projectionMatrix, componentData);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    ImGui::End();
                }

                configuration.getChild(L"editor").attributes[L"enabled"] = showSelectionMenu;
            }
        };

        GEK_REGISTER_CONTEXT_USER(EditorCamera);
        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
