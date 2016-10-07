#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\XML.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\Population.hpp"
#include "GEK\Engine\Entity.hpp"
#include "GEK\Engine\Component.hpp"
#include "GEK\Components\Transform.hpp"
#include <concurrent_vector.h>
#include <ppl.h>

namespace Gek
{
	namespace Implementation
    {
        GEK_CONTEXT_USER(Editor, Plugin::Core *)
            , public Plugin::PopulationListener
            , public Plugin::PopulationStep
        {
        private:
            Edit::Population *population;

            bool showSelectionMenu = true;
            uint32_t selectedEntity = 0;
            uint32_t selectedComponent = 0;

        public:
            Editor(Context *context, Plugin::Core *core)
                : ContextRegistration(context)
                , population(dynamic_cast<Edit::Population *>(core->getPopulation()))
            {
                GEK_REQUIRE(population);
                population->addListener(this);
                population->addStep(this, -1);
            }

            ~Editor(void)
            {
                population->removeStep(this);
                population->removeListener(this);
            }

            // Plugin::PopulationListener
            void onLoadBegin(void)
            {
                selectedEntity = 0;
                selectedComponent = 0;
            }

            void onLoadSucceeded(void)
            {
            }

            void onLoadFailed(void)
            {
            }

            // Plugin::PopulationStep
            void onUpdate(uint32_t order, State state)
            {
                if (showSelectionMenu)
                {
                    ImGui::Begin("Edit Menu", &showSelectionMenu, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysUseWindowPadding);
                    ImGui::Dummy(ImVec2(350, 0));

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
                    auto &entityMap = population->getEntityMap();
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
                                //component->showEditor(ImGui::GetCurrentContext(), viewMatrix, projectionMatrix, componentData);
                            }
                        }
                    }

                    ImGui::End();
                }
            }
        };

        GEK_REGISTER_CONTEXT_USER(Editor);
    }; // namespace Implementation
}; // namespace Gek
