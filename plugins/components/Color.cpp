#include "GEK\Components\Color.hpp"
#include "GEK\Context\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"

namespace Gek
{
    namespace Components
    {
        void Color::save(Xml::Leaf &componentData) const
        {
            componentData.text = value;
        }

        void Color::load(const Xml::Leaf &componentData)
        {
            value = loadText(componentData, Math::Color::White);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Color)
        , public Plugin::ComponentMixin<Components::Color, Editor::Component>
    {
    public:
        Color(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &colorComponent = *dynamic_cast<Components::Color *>(data);
            ImGui::ColorEdit4("Color", colorComponent.value.data);
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"color";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek