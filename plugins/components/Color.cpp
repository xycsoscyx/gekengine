#include "GEK\Components\Color.hpp"
#include "GEK\Utility\ContextUser.hpp"
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
        , public Plugin::ComponentMixin<Components::Color, Edit::Component>
    {
    public:
        Color(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &colorComponent = *dynamic_cast<Components::Color *>(data);
            ImGui::ColorEdit4("Color", colorComponent.value.data);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, 0);
        }

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, 0);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"color";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek