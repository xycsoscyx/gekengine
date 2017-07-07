#include "GEK/Components/Color.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(Color, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Color, Edit::Component>
    {
    public:
        Color(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Color const * const data, JSON::Object &componentData) const
        {
            componentData = JSON::Make(data->value);
        }

        void load(Components::Color * const data, JSON::Reference componentData)
        {
            data->value = parse(componentData, Math::Float4::White);
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);
            ImGui::PushItemWidth(-1.0f);

            auto &colorComponent = *dynamic_cast<Components::Color *>(data);

            ImGui::AlignFirstTextHeightToWidgets();
            ImGui::Text("Color");
            ImGui::SameLine();
            changed |= ImGui::ColorCombo("##color", (ImVec4 *)&colorComponent.value, true, ImGui::GetWindowContentRegionWidth());

            ImGui::PopItemWidth();
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek