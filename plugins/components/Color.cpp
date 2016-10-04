#include "GEK\Components\Color.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include <imgui.h>

namespace Gek
{
    namespace Components
    {
        void Color::save(Plugin::Population::ComponentDefinition &componentData) const
        {
            saveParameter(componentData, nullptr, value);
        }

        void Color::load(const Plugin::Population::ComponentDefinition &componentData)
        {
            value = loadParameter(componentData, nullptr, Math::Color::White);
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
            auto &colorComponent = *dynamic_cast<Components::Color *>(data);

            ImGui::SetCurrentContext(guiContext);

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