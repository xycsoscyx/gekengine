#include "GEK/Components/Color.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(Color, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Color, Edit::Component>
    {
    public:
        Color(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(const Components::Color *data, JSON::Object &componentData) const
        {
            componentData = data->value;
        }

        void load(Components::Color *data, const JSON::Object &componentData)
        {
            if (componentData.is_string())
            {
                data->value = Evaluator::Get<Math::Float4>(population->getShuntingYard(), componentData.as_cstring());
            }
            else
            {
                data->value = Math::Float4::White;
            }
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
    };

    GEK_REGISTER_CONTEXT_USER(Color);
}; // namespace Gek