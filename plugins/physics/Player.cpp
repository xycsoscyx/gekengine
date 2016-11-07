#include "GEK\Math\Common.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\Core.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Components\Transform.hpp"
#include "GEK\Newton\Base.hpp"
#include <algorithm>
#include <memory>

#include <Newton.h>

namespace Gek
{
    namespace Components
    {
        void Player::save(JSON::Object &componentData) const
        {
			componentData.set(L"height", height);
			componentData.set(L"outer_radius", outerRadius);
			componentData.set(L"inner_radius", innerRadius);
			componentData.set(L"stair_step", stairStep);
        }

        void Player::load(const JSON::Object &componentData)
        {
            if (componentData.is_object())
            {
                height = componentData.get(L"height", 6.0f).as<float>();
                outerRadius = componentData.get(L"outer_radius", 1.5f).as<float>();
                innerRadius = componentData.get(L"inner_radius", 0.5f).as<float>();
                stairStep = componentData.get(L"stair_step", 1.0f).as<float>();
            }
        }
    }; // namespace Components

    namespace Newton
    {
        GEK_CONTEXT_USER(Player)
            , public Plugin::ComponentMixin<Components::Player, Edit::Component>
        {
        public:
            Player(Context *context)
                : ContextRegistration(context)
            {
            }

            // Edit::Component
            void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &playerComponent = *dynamic_cast<Components::Player *>(data);
                ImGui::InputFloat("Height", &playerComponent.height, 1.0f, 10.0f, 3, flags);
                ImGui::InputFloat("Outer Radius", &playerComponent.outerRadius, 1.0f, 10.0f, 3, flags);
                ImGui::InputFloat("Inner Radius", &playerComponent.innerRadius, 1.0f, 10.0f, 3, flags);
                ImGui::InputFloat("Stair Step", &playerComponent.stairStep, 1.0f, 10.0f, 3, flags);
                ImGui::SetCurrentContext(nullptr);
            }

            void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
            {
                ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
            }

            void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
            {
                ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"player";
            }
        };

        GEK_REGISTER_CONTEXT_USER(Player)
    }; // namespace Newton
}; // namespace Gek