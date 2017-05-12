#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Newton
    {
        GEK_CONTEXT_USER(Static, Plugin::Population *)
            , public Plugin::ComponentMixin<Components::Static, Edit::Component>
        {
        public:
            Static(Context *context, Plugin::Population *population)
                : ContextRegistration(context)
                , ComponentMixin(population)
            {
            }

            // Plugin::Component
            void save(Components::Static const * const data, JSON::Object &componentData) const
            {
                componentData.set("group"s, data->group);
            }

            void load(Components::Static * const data, const JSON::Object &componentData)
            {
                data->group = getValue(componentData, "group"s, "default"s);
            }

            // Edit::Component
            bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &staticComponent = *dynamic_cast<Components::Static *>(data);
                bool changed =
                    GUI::InputString("Group", staticComponent.group, ImGuiInputTextFlags_EnterReturnsTrue);
                    ImGui::SetCurrentContext(nullptr);
                return changed;
            }

            void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
            }

            bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Static)
    }; // namespace Newton
}; // namespace Gek