#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
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
            void save(Components::Physical const * const data, JSON::Object &componentData) const
            {
                componentData.set("mass", data->mass);
            }

            void load(Components::Physical * const data, JSON::Reference componentData)
            {
                data->mass = parse(componentData.get("mass"), 0.0f);
            }

            // Edit::Component
            bool onUserInterface(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);
                bool changed =
                    ImGui::InputFloat("Mass", &physicalComponent.mass, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::SetCurrentContext(nullptr);
                return changed;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek