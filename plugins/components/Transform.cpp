#include "GEK\Components\Transform.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include <ImGuizmo.h>

namespace Gek
{
    namespace Components
    {
        void Transform::save(Xml::Leaf &componentData) const
        {
            componentData.attributes[L"position"] = position;
            componentData.attributes[L"rotation"] = rotation;
            componentData.attributes[L"scale"] = scale;
        }

        void Transform::load(const Xml::Leaf &componentData)
        {
            position = componentData.getValue(L"position", Math::Float3::Zero);
            rotation = componentData.getValue(L"rotation", Math::Quaternion::Identity);
            scale = componentData.getValue(L"scale", Math::Float3::One);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(Transform)
        , public Plugin::ComponentMixin<Components::Transform, Editor::Component>
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

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
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

            ImGui::InputFloat3("Position", transformComponent.position.data, 3);
            ImGui::InputFloat4("Rotation", transformComponent.rotation.data, 3);
            ImGui::InputFloat3("Scale", transformComponent.scale.data, 3);

            ImGui::NewLine();
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
            transformComponent.rotation = matrix.getQuaternion();
            transformComponent.position = matrix.translation;
            transformComponent.scale = matrix.getScaling();
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"transform";
        }
    };

    GEK_REGISTER_CONTEXT_USER(Transform);
}; // namespace Gek