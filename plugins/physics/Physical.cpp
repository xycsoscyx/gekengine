#include "GEK\Newton\Base.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Math\Common.hpp"

namespace Gek
{
    namespace Components
    {
        void Physical::save(Xml::Leaf &componentData) const
        {
            componentData.text = mass;
        }

        void Physical::load(const Xml::Leaf &componentData)
        {
            mass = loadText(componentData, 0.0f);
        }
    }; // namespace Components

    namespace Newton
    {
        GEK_CONTEXT_USER(Physical)
            , public Plugin::ComponentMixin<Components::Physical, Edit::Component>
        {
        public:
            Physical(Context *context)
                : ContextRegistration(context)
            {
            }

            // Edit::Component
            void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);
                ImGui::InputFloat("Mass", &physicalComponent.mass, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                ImGui::SetCurrentContext(nullptr);
            }

            // Plugin::Component
            const wchar_t * const getName(void) const
            {
                return L"physical";
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek