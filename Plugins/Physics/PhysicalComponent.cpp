#include "API/Engine/ComponentMixin.hpp"
#include "API/Engine/Editor.hpp"
#include "GEK/Math/Common.hpp"
#include "GEK/Physics/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    namespace Physics
    {
        GEK_CONTEXT_USER(Physical, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Physical, Edit::Component>
        {
          private:
            int selectedShape = 0;

          public:
            Physical(Context * context, Plugin::Population * population)
                : ContextRegistration(context), ComponentMixin(population)
            {
            }

            // Plugin::Component
            void save(Components::Physical const *const data, JSON::Object &exportData) const
            {
                exportData["mass"] = data->mass;
            }

            void load(Components::Physical *const data, JSON::Object const &importData)
            {
                getContext()->log(Context::Info, "Loading Physical component with {}", importData.dump());
                data->mass = evaluate(importData, "mass", 0.0f);
            }

            // Edit::Component
            bool onUserInterface(ImGuiContext *const guiContext, Plugin::Entity *const entity, Plugin::Component::Data *data)
            {
                bool changed = false;
                ImGui::SetCurrentContext(guiContext);

                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);

                changed |= editorElement("Mass", [&](void) -> bool
                                         { return ImGui::InputFloat("##mass", &physicalComponent.mass, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank); });

                ImGui::SetCurrentContext(nullptr);
                return changed;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Physics
}; // namespace Gek