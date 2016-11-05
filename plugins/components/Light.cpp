#include "GEK\Components\Light.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Math\Common.hpp"

namespace Gek
{
    namespace Components
    {
        void PointLight::save(JSON::Object &componentData) const
        {
            JSON::setMember(componentData, L"range", range);
			JSON::setMember(componentData, L"radius", radius);
        }

        void PointLight::load(const JSON::Object &componentData)
        {
            range = JSON::getMember(componentData, L"range", 0.0f);
            radius = JSON::getMember(componentData, L"radius", 0.0f);
        }

        void SpotLight::save(JSON::Object &componentData) const
        {
            componentData[L"range"] = range;
            componentData[L"radius"] = radius;
            componentData[L"inner_angle"] = Math::convertRadiansToDegrees(std::acos(innerAngle) * 2.0f);
            componentData[L"outer_angle"] = Math::convertRadiansToDegrees(std::acos(outerAngle) * 2.0f);
            componentData[L"cone_falloff"] = coneFalloff;
        }

        void SpotLight::load(const JSON::Object &componentData)
        {
            range = JSON::getMember(componentData, L"range", 0.0f);
            radius = JSON::getMember(componentData, L"radius", 0.0f);
            innerAngle = std::cos(Math::convertDegreesToRadians(JSON::getMember(componentData, L"inner_angle", 0.0f) * 0.5f));
            outerAngle = std::cos(Math::convertDegreesToRadians(JSON::getMember(componentData, L"outer_angle", 0.0f) * 0.5f));
            coneFalloff = JSON::getMember(componentData, L"cone_falloff", 0.0f);
        }

        void DirectionalLight::save(JSON::Object &componentData) const
        {
        }

        void DirectionalLight::load(const JSON::Object &componentData)
        {
        }
    }; // namespace Components

    GEK_CONTEXT_USER(PointLight)
        , public Plugin::ComponentMixin<Components::PointLight, Edit::Component>
    {
    public:
        PointLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &pointLightComponent = *dynamic_cast<Components::PointLight *>(data);
            ImGui::InputFloat("Range", &pointLightComponent.range, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Radius", &pointLightComponent.radius, 1.0f, 10.0f, 3, flags);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"point_light";
        }
    };

    GEK_CONTEXT_USER(SpotLight)
        , public Plugin::ComponentMixin<Components::SpotLight, Edit::Component>
    {
    public:
        SpotLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &spotLightComponent = *dynamic_cast<Components::SpotLight *>(data);
            ImGui::InputFloat("Range", &spotLightComponent.range, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Radius", &spotLightComponent.radius, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Inner Angle", &spotLightComponent.innerAngle, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Outer Angle", &spotLightComponent.outerAngle, 1.0f, 10.0f, 3, flags);
            ImGui::InputFloat("Cone Falloff", &spotLightComponent.coneFalloff, 1.0f, 10.0f, 3, flags);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }

        // Plugin::Component
        const wchar_t * const getName(void) const
        {
            return L"spot_light";
        }
    };

    GEK_CONTEXT_USER(DirectionalLight)
        , public Plugin::ComponentMixin<Components::DirectionalLight, Edit::Component>
    {
    public:
        DirectionalLight(Context *context)
            : ContextRegistration(context)
        {
        }

        // Edit::Component
        void ui(ImGuiContext *guiContext, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &directionalLightComponent = *dynamic_cast<Components::DirectionalLight *>(data);
            ImGui::SetCurrentContext(nullptr);
        }

        void show(ImGuiContext *guiContext, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_ReadOnly);
        }

        void edit(ImGuiContext *guiContext, const Math::SIMD::Float4x4 &viewMatrix, const Math::SIMD::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
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