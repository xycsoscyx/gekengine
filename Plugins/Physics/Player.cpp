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
            void save(const Components::Player *data, JSON::Object &componentData) const
            {
                componentData.set(L"height", data->height);
                componentData.set(L"outerRadius", data->outerRadius);
                componentData.set(L"innerRadius", data->innerRadius);
                componentData.set(L"stairStep", data->stairStep);
            }

            void load(Components::Player *data, const JSON::Object &componentData)
            {
                data->height = getValue(componentData, L"height", 0.0f);
                data->outerRadius = getValue(componentData, L"outerRadius", 0.0f);
                data->innerRadius = getValue(componentData, L"innerRadius", 0.0f);
                data->stairStep = getValue(componentData, L"stairStep", 0.0f);
            }

            // Edit::Component
            bool ui(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &playerComponent = *dynamic_cast<Components::Player *>(data);
                bool changed = 
                    ImGui::Gek::InputFloat("Height", &playerComponent.height, 1.0f, 10.0f, 3, flags) |
                    ImGui::Gek::InputFloat("Outer Radius", &playerComponent.outerRadius, 1.0f, 10.0f, 3, flags) |
                    ImGui::Gek::InputFloat("Inner Radius", &playerComponent.innerRadius, 1.0f, 10.0f, 3, flags) |
                    ImGui::Gek::InputFloat("Stair Step", &playerComponent.stairStep, 1.0f, 10.0f, 3, flags);
                ImGui::SetCurrentContext(nullptr);
                return changed;
            }

            void show(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data)
            {
                ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
            }

            bool edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Entity *entity, Plugin::Component::Data *data)
            {
                return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Player)
    }; // namespace Newton
}; // namespace Gek