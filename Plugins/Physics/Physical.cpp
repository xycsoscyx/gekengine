#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Newton
    {
        GEK_CONTEXT_USER(Physical, Plugin::Population *)
            , public Plugin::ComponentMixin<Components::Physical, Edit::Component>
        {
        public:
            Physical(Context *context, Plugin::Population *population)
                : ContextRegistration(context)
                , ComponentMixin(population)
            {
            }

            // Plugin::Component
            void save(const Components::Physical *data, JSON::Object &componentData) const
            {
                componentData.set(L"mass", data->mass);
            }

            void load(Components::Physical *data, const JSON::Object &componentData)
            {
                data->mass = getValue(componentData, L"mass", 0.0f);
            }

            // Edit::Component
            bool ui(ImGuiContext *guiContext, Plugin::Entity *entity, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);
                bool changed =
                    ImGui::Gek::InputFloat("Mass", &physicalComponent.mass, 1.0f, 10.0f, 3, flags);
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

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek