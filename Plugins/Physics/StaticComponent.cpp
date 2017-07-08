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
                componentData.set("group", data->group);
            }

            void load(Components::Static * const data, JSON::Reference componentData)
            {
				static const std::string DefaultGroup("default");
                data->group = parse(componentData.get("group"), DefaultGroup);
            }

            // Edit::Component
            bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                bool changed = false;
                ImGui::SetCurrentContext(guiContext);

                auto &staticComponent = *dynamic_cast<Components::Static *>(data);

                changed |= editorElement("Group", [&](void) -> bool
                {
                    return UI::InputString("##group", staticComponent.group, ImGuiInputTextFlags_EnterReturnsTrue);
                });

                ImGui::SetCurrentContext(nullptr);
                return changed;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Static)
    }; // namespace Newton
}; // namespace Gek