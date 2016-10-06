#include "GEK\Newton\Base.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

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
            , public Plugin::ComponentMixin<Components::Physical, Editor::Component>
        {
        public:
            Physical(Context *context)
                : ContextRegistration(context)
            {
            }

            // Editor::Component
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