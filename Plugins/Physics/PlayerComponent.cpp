#include "GEK/Math/Common.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/Core.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Components/Transform.hpp"
#include "GEK/Newton/Base.hpp"
#include <algorithm>
#include <memory>

#include <Newton.h>

namespace Gek
{
    namespace Newton
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
            void save(Components::Player const * const data, JSON::Object &componentData) const
            {
                componentData.set("height", data->height);
                componentData.set("outerRadius", data->outerRadius);
                componentData.set("innerRadius", data->innerRadius);
                componentData.set("stairStep", data->stairStep);
            }

            void load(Components::Player * const data, JSON::Reference componentData)
            {
                data->height = parse(componentData.get("height"), 0.0f);
                data->outerRadius = parse(componentData.get("outerRadius"), 0.0f);
                data->innerRadius = parse(componentData.get("innerRadius"), 0.0f);
                data->stairStep = parse(componentData.get("stairStep"), 0.0f);
            }

            // Edit::Component
            bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &playerComponent = *dynamic_cast<Components::Player *>(data);
                bool changed = 
                    ImGui::InputFloat("Height", &playerComponent.height, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                    ImGui::InputFloat("Outer Radius", &playerComponent.outerRadius, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                    ImGui::InputFloat("Inner Radius", &playerComponent.innerRadius, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                    ImGui::InputFloat("Stair Step", &playerComponent.stairStep, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags);
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

        GEK_REGISTER_CONTEXT_USER(Player)
    }; // namespace Newton
}; // namespace Gek