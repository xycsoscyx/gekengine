#include "GEK\Components\Light.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\Engine\ComponentMixin.hpp"
#include "GEK\Utility\String.hpp"

namespace Gek
{
    namespace Components
    {
        void PointLight::save(JSON::Object &componentData) const
        {
            componentData.set(L"range", range);
			componentData.set(L"radius", radius);
        }

        void PointLight::load(const JSON::Object &componentData)
        {
            if (componentData.is_object())
            {
                range = componentData.get(L"range", 0.0f).as<float>();
                radius = componentData.get(L"radius", 0.0f).as<float>();
            }
        }

        void SpotLight::save(JSON::Object &componentData) const
        {
            componentData.set(L"range",  range);
            componentData.set(L"radius",  radius);
            componentData.set(L"innerAngle",  Math::convertRadiansToDegrees(std::acos(innerAngle) * 2.0f));
            componentData.set(L"outerAngle",  Math::convertRadiansToDegrees(std::acos(outerAngle) * 2.0f));
            componentData.set(L"coneFalloff",  coneFalloff);
        }

        void SpotLight::load(const JSON::Object &componentData)
        {
            if (componentData.is_object())
            {
                range = componentData.get(L"range", 0.0f).as<float>();
                radius = componentData.get(L"radius", 0.0f).as<float>();
                innerAngle = std::cos(Math::convertDegreesToRadians(componentData.get(L"innerAngle", 0.0f).as<float>() * 0.5f));
                outerAngle = std::cos(Math::convertDegreesToRadians(componentData.get(L"outerAngle", 0.0f).as<float>() * 0.5f));
                coneFalloff = componentData.get(L"coneFalloff", 0.0f).as<float>();
            }
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
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek