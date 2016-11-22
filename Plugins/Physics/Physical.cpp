#include "GEK/Newton/Base.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Math/Common.hpp"

namespace Gek
{
    namespace Components
    {
        void Physical::save(JSON::Object &componentData) const
        {
            componentData.set(L"mass", mass);
        }

        void Physical::load(const JSON::Object &componentData)
        {
            if (componentData.is_object())
            {
                mass = componentData.get(L"mass", 0.0f).as<float>();
            }
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
            void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
            {
                ImGui::SetCurrentContext(guiContext);
                auto &physicalComponent = *dynamic_cast<Components::Physical *>(data);
                ImGui::InputFloat("Mass", &physicalComponent.mass, 1.0f, 10.0f, 3, flags);
                ImGui::SetCurrentContext(nullptr);
            }

            void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
            {
                ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
            }

            void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
            {
                ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Physical)
    }; // namespace Newton
}; // namespace Gek