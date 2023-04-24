#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/Core.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Physics/Base.hpp"
#include <algorithm>
#include <memory>

namespace Gek
{
    namespace Physics
    {
        GEK_CONTEXT_USER(Player, Plugin::Population *)
            , public Plugin::ComponentMixin<Components::Player, Edit::Component>
        {
        public:
            Player(Context *context, Plugin::Population *population)
                : ContextRegistration(context)
                , ComponentMixin(population)
            {
            }

            // Plugin::Component
            void save(Components::Player const * const data, JSON::Object &exportData) const
            {
                exportData["height"] = data->height;
                exportData["outerRadius"] = data->outerRadius;
                exportData["innerRadius"] = data->innerRadius;
                exportData["stairStep"] = data->stairStep;
            }

            void load(Components::Player * const data, JSON::Object const &importData)
            {
                data->height = evaluate(importData["height"], 0.0f);
                data->outerRadius = evaluate(importData["outerRadius"], 0.0f);
                data->innerRadius = evaluate(importData["innerRadius"], 0.0f);
                data->stairStep = evaluate(importData["stairStep"], 0.0f);
            }

            // Edit::Component
            bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
            {
                bool changed = false;
                ImGui::SetCurrentContext(guiContext);

                auto &playerComponent = *dynamic_cast<Components::Player *>(data);

                changed |= editorElement("Height", [&](void) -> bool
                {
                    return ImGui::InputFloat("##height", &playerComponent.height, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                });

                changed |= editorElement("Outer Radius", [&](void) -> bool
                {
                    return ImGui::InputFloat("##outerRadius", &playerComponent.outerRadius, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                });

                changed |= editorElement("Inner Radius", [&](void) -> bool
                {
                    return ImGui::InputFloat("##innerRadius", &playerComponent.innerRadius, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                });

                changed |= editorElement("Stair Step", [&](void) -> bool
                {
                    return ImGui::InputFloat("##stairStep", &playerComponent.stairStep, 1.0f, 10.0f, "%.3f", ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                });

                ImGui::SetCurrentContext(nullptr);
                return changed;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Player)
    }; // namespace Physics
}; // namespace Gek