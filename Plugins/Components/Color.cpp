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
        void save(const Components::Color *data, JSON::Object &componentData) const
        {
            componentData = data->value;
        }

        void load(Components::Color *data, const JSON::Object &componentData)
        {
            data->value = getValue(componentData, Math::Float4::White);
        }

        // Edit::Component
        bool ui(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &colorComponent = *dynamic_cast<Components::Color *>(data);
            bool changed = ImGui::ColorCombo("Color", (ImVec4 *)&colorComponent.value, true, ImGui::GetWindowContentRegionWidth());
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, 0);
        }

        bool edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Entity *entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, 0);
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek