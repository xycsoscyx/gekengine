#include "GEK\Components\Filter.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"

namespace Gek
{
    namespace Components
    {
        void Filter::save(Xml::Leaf &componentData) const
        {
            componentData.text.join(list, L',');
        }

        void Filter::load(const Xml::Leaf &componentData)
        {
            list = componentData.text.split(L',');
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Filter)
        , public Plugin::ComponentMixin<Components::Filter, Edit::Component>
    {
    public:
        Filter(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &filterComponent = *dynamic_cast<Components::Filter *>(data);
            if (ImGui::ListBoxHeader("##Filters", filterComponent.list.size(), 5))
            {
                ImGuiListClipper clipper(filterComponent.list.size(), ImGui::GetTextLineHeightWithSpacing());
                while (clipper.Step())
                {
                    for (int filterIndex = clipper.DisplayStart; filterIndex < clipper.DisplayEnd; ++filterIndex)
                    {
                        ImGui::PushID(filterIndex);
                        ImGui::PushItemWidth(-1);
                        ImGui::InputText("##", filterComponent.list[filterIndex], flags);
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

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, 0);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"filter";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Filter);
}; // namespace Gek