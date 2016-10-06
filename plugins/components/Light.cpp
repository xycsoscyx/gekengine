#include "GEK\Components\Light.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Engine\ComponentMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Math\Common.h"

namespace Gek
{
    namespace Components
    {
        void PointLight::save(Xml::Leaf &componentData) const
        {
            componentData.attributes[L"range"] = range;
            componentData.attributes[L"radius"] = radius;
            componentData.attributes[L"intensity"] = intensity;
        }

        void PointLight::load(const Xml::Leaf &componentData)
        {
            range = loadAttribute(componentData, L"range", 0.0f);
            radius = loadAttribute(componentData, L"radius", 0.0f);
            intensity = loadAttribute(componentData, L"intensity", 0.0f);
        }

        void SpotLight::save(Xml::Leaf &componentData) const
        {
            componentData.attributes[L"range"] = range;
            componentData.attributes[L"radius"] = radius;
            componentData.attributes[L"intensity"] = intensity;
            componentData.attributes[L"inner_angle"] = Math::convertRadiansToDegrees(std::acos(innerAngle) * 2.0f);
            componentData.attributes[L"outer_angle"] = Math::convertRadiansToDegrees(std::acos(outerAngle) * 2.0f);
            componentData.attributes[L"falloff"] = falloff;
        }

        void SpotLight::load(const Xml::Leaf &componentData)
        {
            range = loadAttribute(componentData, L"range", 0.0f);
            radius = loadAttribute(componentData, L"radius", 0.0f);
            intensity = loadAttribute(componentData, L"intensity", 0.0f);
            innerAngle = std::cos(Math::convertDegreesToRadians(loadAttribute(componentData, L"inner_angle", 0.0f) * 0.5f));
            outerAngle = std::cos(Math::convertDegreesToRadians(loadAttribute(componentData, L"outer_angle", 0.0f) * 0.5f));
            falloff = loadAttribute(componentData, L"falloff", 0.0f);
        }

        void DirectionalLight::save(Xml::Leaf &componentData) const
        {
            componentData.attributes[L"intensity"] = intensity;
        }

        void DirectionalLight::load(const Xml::Leaf &componentData)
        {
            intensity = loadAttribute(componentData, L"intensity", 0.0f);
        }
    }; // namespace Components

    GEK_CONTEXT_USER(PointLight)
        , public Plugin::ComponentMixin<Components::PointLight, Editor::Component>
    {
    public:
        PointLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &pointLightComponent = *dynamic_cast<Components::PointLight *>(data);
            ImGui::InputFloat("Range", &pointLightComponent.range, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Radius", &pointLightComponent.radius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Intensity", &pointLightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"point_light";
        }
    };

    GEK_CONTEXT_USER(SpotLight)
        , public Plugin::ComponentMixin<Components::SpotLight, Editor::Component>
    {
    public:
        SpotLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &spotLightComponent = *dynamic_cast<Components::SpotLight *>(data);
            ImGui::InputFloat("Range", &spotLightComponent.range, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Radius", &spotLightComponent.radius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Intensity", &spotLightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Inner Angle", &spotLightComponent.innerAngle, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Outer Angle", &spotLightComponent.outerAngle, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::InputFloat("Falloff", &spotLightComponent.falloff, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spot_light";
        }
    };

    GEK_CONTEXT_USER(DirectionalLight)
        , public Plugin::ComponentMixin<Components::DirectionalLight, Editor::Component>
    {
    public:
        DirectionalLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Editor::Component
        void showEditor(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &directionalLightComponent = *dynamic_cast<Components::DirectionalLight *>(data);
            ImGui::InputFloat("Intensity", &directionalLightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            ImGui::SetCurrentContext(nullptr);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"directional_light";
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek