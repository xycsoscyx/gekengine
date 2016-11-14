#include "GEK\Components\Transform.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"
#include <ImGuizmo.h>

namespace Gek
{
    namespace Components
    {
        void Transform::save(JSON::Object &componentData) const
        {
            componentData.set(L"position", position);
			componentData.set(L"rotation", rotation);
			componentData.set(L"scale", scale);
        }

        void Transform::load(const JSON::Object &componentData)
        {
            if (componentData.is_object())
            {
                position = componentData.get(L"position", Math::Float3::Zero).as<Math::Float3>();
                rotation = componentData.get(L"rotation", Math::Quaternion::Identity).as<Math::Quaternion>();
                scale = componentData.get(L"scale", Math::Float3::One).as<Math::Float3>();
            }
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Transform)
        , public Plugin::ComponentMixin<Components::Transform, Edit::Component>
    {
    private:
        ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
        bool useSnap = true;
        Math::Float3 snapPosition = (1.0f / 12.0f);
        float snapRotation = 10.0f;
        float snapScale = (1.0f / 10.0f);

    public:
        Transform(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &transformComponent = *dynamic_cast<Components::Transform *>(data);
            ImGui::InputFloat3("Position", transformComponent.position.data, 4, ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat4("Rotation", transformComponent.rotation.data, 4, ImGuiInputTextFlags_ReadOnly);
            ImGui::InputFloat3("Scale", transformComponent.scale.data, 4, ImGuiInputTextFlags_ReadOnly);
            ImGui::SetCurrentContext(nullptr);
        }

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &transformComponent = *dynamic_cast<Components::Transform *>(data);
            if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
            {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }

            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
            {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }

            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
            {
                currentGizmoOperation = ImGuizmo::SCALE;
            }

            ImGui::Separator();
            ImGui::InputFloat3("Position", transformComponent.position.data, 4);
            ImGui::InputFloat4("Rotation", transformComponent.rotation.data, 4);
            ImGui::InputFloat3("Scale", transformComponent.scale.data, 4);

            ImGui::Separator();
            ImGui::Checkbox("Snap", &useSnap);
            ImGui::SameLine();

            float *snap = nullptr;
            switch (currentGizmoOperation)
            {
            case ImGuizmo::TRANSLATE:
                ImGui::InputFloat3("##Units", snapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = snapPosition.data;
                break;

            case ImGuizmo::ROTATE:
                ImGui::InputFloat("##Degrees", &snapRotation, 10.0f, 90.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = &snapRotation;
                break;

            case ImGuizmo::SCALE:
                ImGui::InputFloat("##Size", &snapScale, (1.0f / 10.0f), 1.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = &snapScale;
                break;
            };

            auto matrix(transformComponent.getMatrix());

            ImGuizmo::BeginFrame();
            ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, currentGizmoOperation, ImGuizmo::WORLD, matrix.data, nullptr, snap);
            transformComponent.rotation = matrix.getRotation();
            transformComponent.position = matrix.translation.xyz;
            transformComponent.scale = matrix.getScaling();

            ImGui::SetCurrentContext(nullptr);
        }
    };

    GEK_REGISTER_CONTEXT_USER(Transform);
}; // namespace Gek