#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Newton
    {
        GEK_CONTEXT_USER(Physical, Plugin::Population *)
            , public Plugin::ComponentMixin<Components::Physical, Edit::Component>
        {
        public:
            Physical(Context *context, Plugin::Population *population)
                : ContextRegistration(context)
                , ComponentMixin(population)
            {
            }

            // Plugin::Component
            void save(Components::Physical const * const data, JSON &exportData) const
            {
                exportData["mass"sv] = data->mass;
            }

            void load(Components::Physical * const data, JSON const &importData)
            {
                data->mass = evaluate(importData.getMember("mass"sv), 0.0f);
            }

            // Edit::Component
            bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                bool changed = false;
                ImGui::SetCurrentContext(guiContext);

                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);

                changed |= editorElement("Mass"sv, [&](void) -> bool
                {
                    return ImGui::InputFloat("##mass", &physicalComponent.mass, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                });

                ImGui::SetCurrentContext(nullptr);
                return changed;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek