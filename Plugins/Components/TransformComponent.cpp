#include "GEK/Components/Transform.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Population.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(Transform, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Transform, Edit::Component>
    {
    public:
        Transform(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Transform const * const data, JSON &exportData) const
        {
			exportData["position"] = JSON::Array({ data->position.x, data->position.y, data->position.z });
			exportData["rotation"] = JSON::Array({ data->rotation.x, data->rotation.y, data->rotation.z, data->rotation.w });
        }

        void load(Components::Transform * const data, JSON const &importData)
        {
            data->position = evaluate(importData.get("position"), Math::Float3::Zero);
            data->rotation = evaluate(importData.get("rotation"), Math::Quaternion::Identity);
            LockedWrite{ std::cout } << "Position: [" << data->position.x << ", " << data->position.y << ", " << data->position.z << "]";
		}

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &transformComponent = *dynamic_cast<Components::Transform *>(data);

            changed |= editorElement("Position", [&](void) -> bool
            {
                return ImGui::InputFloat3("##position", transformComponent.position.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Rotation", [&](void) -> bool
            {
                return ImGui::InputFloat4("##rotation", transformComponent.rotation.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Scale", [&](void) -> bool
            {
                return ImGui::InputFloat3("##scale", transformComponent.scale.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
        }
    };

    GEK_REGISTER_CONTEXT_USER(Transform);
}; // namespace Gek