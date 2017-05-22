#include "GEK/Components/Transform.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Engine/Population.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(Transform, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::Transform, Edit::Component>
    {
    private:
        ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
        bool useSnap = true;
        Math::Float3 snapPosition = Math::Float3(1.0f / 12.0f);
        float snapRotation = 10.0f;
        float snapScale = (1.0f / 10.0f);
        bool showEuler = true;
        bool showRadians = false;

    public:
        Transform(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(Components::Transform const * const data, JSON::Object &componentData) const
        {
            componentData.set("position", data->position);
            componentData.set("rotation", data->rotation);
        }

        void load(Components::Transform * const data, const JSON::Object &componentData)
        {
            data->position = getValue(componentData, "position", Math::Float3::Zero);
            data->rotation = getValue(componentData, "rotation", Math::Quaternion::Identity);
            AtomicWriter() << "Position: " << data->position.x << ", " << data->position.y << ", " << data->position.z << std::endl;
		}

        // Edit::Component
        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &transformComponent = *dynamic_cast<Components::Transform *>(data);
            GUI::InputFloat3("Position", transformComponent.position.data, 4, ImGuiInputTextFlags_ReadOnly);

            ImGui::Text("Rotation: ");
            ImGui::SameLine();
            ImGui::Checkbox("Euler", &showEuler);
            if (showEuler)
            {
                ImGui::SameLine();
                if (ImGui::RadioButton("Degrees", !showRadians))
                {
                    showRadians = false;
                }

                ImGui::SameLine();
                if (ImGui::RadioButton("Radians", showRadians))
                {
                    showRadians = true;
                }

                auto euler(transformComponent.rotation.getEuler());
                if (!showRadians)
                {
                    euler.x = Math::RadiansToDegrees(euler.x);
                    euler.y = Math::RadiansToDegrees(euler.y);
                    euler.z = Math::RadiansToDegrees(euler.z);
                }

                ImGui::InputFloat3("##RotationEuler", euler.data, 4, ImGuiInputTextFlags_ReadOnly);
            }
            else
            {
                ImGui::InputFloat4("##RotationQuaternion", transformComponent.rotation.data, 4, ImGuiInputTextFlags_ReadOnly);
            }

            GUI::InputFloat3("Scale", transformComponent.scale.data, 4, ImGuiInputTextFlags_ReadOnly);

            ImGui::SetCurrentContext(nullptr);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &transformComponent = *dynamic_cast<Components::Transform *>(data);
            GUI::InputFloat3("Position", transformComponent.position.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);

            ImGui::Text("Rotation: ");
            ImGui::SameLine();
            ImGui::Checkbox("Euler", &showEuler);
            if (showEuler)
            {
                ImGui::SameLine();
                if (ImGui::RadioButton("Degrees", !showRadians))
                {
                    showRadians = false;
                }

                ImGui::SameLine();
                if (ImGui::RadioButton("Radians", showRadians))
                {
                    showRadians = true;
                }

                auto euler(transformComponent.rotation.getEuler());
                if (!showRadians)
                {
                    euler.x = Math::RadiansToDegrees(euler.x);
                    euler.y = Math::RadiansToDegrees(euler.y);
                    euler.z = Math::RadiansToDegrees(euler.z);
                }

                if (ImGui::InputFloat3("##RotationEuler", euler.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank))
                {
                    if (!showRadians)
                    {
                        euler.x = Math::DegreesToRadians(euler.x);
                        euler.y = Math::DegreesToRadians(euler.y);
                        euler.z = Math::DegreesToRadians(euler.z);
                    }

                    transformComponent.rotation = Math::Quaternion::FromEuler(euler);
                }
            }
            else
            {
                ImGui::InputFloat4("##RotationQuaternion", transformComponent.rotation.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            }

            ImGui::Separator();
            ImGui::Text("Gizmo Affects:");            
            if (ImGui::RadioButton("Translation", currentGizmoOperation == ImGuizmo::TRANSLATE))
            {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }

            if (ImGui::RadioButton("Rotation", currentGizmoOperation == ImGuizmo::ROTATE))
            {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }

            GUI::InputFloat3("Scale", transformComponent.scale.data, 4, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);

            ImGui::Separator();
            ImGui::Checkbox("Enable Snap", &useSnap);

            float *snap = nullptr;
            switch (currentGizmoOperation)
            {
            case ImGuizmo::TRANSLATE:
                GUI::InputFloat3("Units", snapPosition.data, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = snapPosition.data;
                break;

            case ImGuizmo::ROTATE:
                GUI::InputFloat("Degrees", &snapRotation, 10.0f, 90.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = &snapRotation;
                break;

            case ImGuizmo::SCALE:
                GUI::InputFloat("Size", &snapScale, (1.0f / 10.0f), 1.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
                snap = &snapScale;
                break;
            };

            auto matrix(transformComponent.getMatrix());

            ImGuizmo::BeginFrame();
            ImGuizmo::Manipulate(viewMatrix.data, projectionMatrix.data, currentGizmoOperation, ImGuizmo::WORLD, matrix.data, nullptr, snap);
            ImGui::SetCurrentContext(nullptr);

            auto rotation(matrix.getRotation());
            auto position(matrix.translation.xyz);
            if (position != transformComponent.position || rotation != transformComponent.rotation)
            {
                transformComponent.rotation = rotation;
                transformComponent.position = position;
                return true;
            }

            return false;
        }
    };

    GEK_REGISTER_CONTEXT_USER(Transform);
}; // namespace Gek