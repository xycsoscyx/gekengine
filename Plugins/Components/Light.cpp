#include "GEK/Components/Light.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Engine/ComponentMixin.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    GEK_CONTEXT_USER(PointLight, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::PointLight, Edit::Component>
    {
    public:
        PointLight(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(const Components::PointLight *data, JSON::Object &componentData) const
        {
            componentData.set(L"range", data->range);
            componentData.set(L"radius", data->radius);
        }

        void load(Components::PointLight *data, const JSON::Object &componentData)
        {
            data->range = getValue(componentData, L"range", 0.0f);
            data->radius = getValue(componentData, L"radius", 0.0f);
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

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }
    };

    GEK_CONTEXT_USER(SpotLight, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::SpotLight, Edit::Component>
    {
    public:
        SpotLight(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
        {
        }

        // Plugin::Component
        void save(const Components::SpotLight *data, JSON::Object &componentData) const
        {
            componentData.set(L"range", data->range);
            componentData.set(L"radius", data->radius);
            componentData.set(L"innerAngle", Math::RadiansToDegrees(std::acos(data->innerAngle) * 2.0f));
            componentData.set(L"outerAngle", Math::RadiansToDegrees(std::acos(data->outerAngle) * 2.0f));
            componentData.set(L"coneFalloff", data->coneFalloff);
        }

        void load(Components::SpotLight *data, const JSON::Object &componentData)
        {
            data->range = getValue(componentData, L"range", 0.0f);
            data->radius = getValue(componentData, L"radius", 0.0f);
            data->innerAngle = std::cos(Math::DegreesToRadians(getValue(componentData, L"innerAngle", 0.0f)));
            data->outerAngle = std::cos(Math::DegreesToRadians(getValue(componentData, L"outerAngle", 0.0f)));
            data->coneFalloff = getValue(componentData, L"coneFalloff", 0.0f);
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

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }
    };

    GEK_CONTEXT_USER(DirectionalLight, Plugin::Population *)
        , public Plugin::ComponentMixin<Components::DirectionalLight, Edit::Component>
    {
    public:
        DirectionalLight(Context *context, Plugin::Population *population)
            : ContextRegistration(context)
            , ComponentMixin(population)
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

        void edit(ImGuiContext *guiContext, const Math::Float4x4 &viewMatrix, const Math::Float4x4 &projectionMatrix, Plugin::Component::Data *data)
        {
            ui(guiContext, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek