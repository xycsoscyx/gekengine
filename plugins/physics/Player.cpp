#include "GEK\Math\Common.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\Core.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Components\Transform.h"
#include "GEK\Newton\Base.h"
#include <algorithm>
#include <memory>

#include <Newton.h>

namespace Gek
{
    namespace Components
    {
        void Player::save(Xml::Leaf &componentData) const
        {
			componentData.attributes[L"height"] = height;
            componentData.attributes[L"outer_radius"] = outerRadius;
            componentData.attributes[L"inner_radius"] = innerRadius;
            componentData.attributes[L"stair_step"] = stairStep;
        }

        void Player::load(const Xml::Leaf &componentData)
        {
			height = componentData.getValue(L"height", 6.0f);
            outerRadius = componentData.getValue(L"outer_radius", 1.5f);
            innerRadius = componentData.getValue(L"inner_radius", 0.5f);
            stairStep = componentData.getValue(L"stair_step", 1.5f);
        }
    }; // namespace Components

    namespace Newton
    {
        GEK_CONTEXT_USER(Player)
            , public Plugin::ComponentMixin<Components::Player, Editor::Component>
        {
        public:
            Player(Context *context)
                : ContextRegistration(context)
            {
            }

            // Editor::Component
            void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &playerComponent = *dynamic_cast<Components::Player *>(data);
                ImGui::InputFloat("Height", &playerComponent.height, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::InputFloat("Outer Radius", &playerComponent.outerRadius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::InputFloat("Inner Radius", &playerComponent.innerRadius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::InputFloat("Stair Step", &playerComponent.stairStep, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::SetCurrentContext(nullptr);
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