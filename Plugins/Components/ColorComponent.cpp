#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/GUI/Utilities.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Color.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(Color, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Color, Edit::Component>
    {
    private:
        int currentMode = ImGuiColorEditFlags_RGB;
        bool useHDR = false;

    public:
        Color(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Color const * const data, JSON &componentData) const
        {
			componentData = JSON::Array({ data->value.x, data->value.y, data->value.z, data->value.w });
        }

        void load(Components::Color * const data, JSON &componentData)
        {
            data->value = parse(componentData, Math::Float4::White);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &colorComponent = *dynamic_cast<Components::Color *>(data);

            UI::CheckButton("  Allow HDR  ", &useHDR);
            ImGui::SameLine();
            auto &style = ImGui::GetStyle();
            float width = (ImGui::GetContentRegionAvailWidth() - style.ItemSpacing.x * 1.0f) / 2.0f;
            UI::RadioButton("RGB", &currentMode, (int)ImGuiColorEditFlags_RGB, ImVec2(width, 0.0f));
            ImGui::SameLine();
            UI::RadioButton("HSV", &currentMode, (int)ImGuiColorEditFlags_HSV, ImVec2(width, 0.0f));

            ImGui::PushItemWidth(-1.0f);
            changed |= ImGui::ColorPicker4("##color", colorComponent.value.data, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float | currentMode | (useHDR ? ImGuiColorEditFlags_HDR : 0));
            ImGui::PopItemWidth();

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek