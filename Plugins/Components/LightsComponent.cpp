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
        void save(Components::PointLight const * const data, JSON::Object &componentData) const
        {
            componentData.set("range", data->range);
            componentData.set("radius", data->radius);
            componentData.set("intensity", data->intensity);
        }

        void load(Components::PointLight * const data, JSON::Reference componentData)
        {
            data->range = parse(componentData.get("range"), 0.0f);
            data->radius = parse(componentData.get("radius"), 0.0f);
            data->intensity = parse(componentData.get("intensity"), 0.0f);
            WriteOutput(std::cout, "Range: %v, Radius: %v, Intensity: %v", data->range, data->radius, data->intensity);
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &lightComponent = *dynamic_cast<Components::PointLight *>(data);
            bool changed =
                GUI::InputFloat("Range", &lightComponent.range, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Radius", &lightComponent.radius, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Intensity", &lightComponent.intensity, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags);
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
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
        void save(Components::SpotLight const * const data, JSON::Object &componentData) const
        {
            componentData.set("range", data->range);
            componentData.set("radius", data->radius);
            componentData.set("intensity", data->intensity);
            componentData.set("innerAngle", Math::RadiansToDegrees(std::acos(data->innerAngle) * 2.0f));
            componentData.set("outerAngle", Math::RadiansToDegrees(std::acos(data->outerAngle) * 2.0f));
            componentData.set("coneFalloff", data->coneFalloff);
        }

        void load(Components::SpotLight * const data, JSON::Reference componentData)
        {
            data->range = parse(componentData.get("range"), 0.0f);
            data->radius = parse(componentData.get("radius"), 0.0f);
            data->intensity = parse(componentData.get("intensity"), 0.0f);
            data->innerAngle = std::cos(Math::DegreesToRadians(parse(componentData.get("innerAngle"), 0.0f)));
            data->outerAngle = std::cos(Math::DegreesToRadians(parse(componentData.get("outerAngle"), 0.0f)));
            data->coneFalloff = parse(componentData.get("coneFalloff"), 0.0f);
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &lightComponent = *dynamic_cast<Components::SpotLight *>(data);
            bool changed = 
                GUI::InputFloat("Range", &lightComponent.range, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Radius", &lightComponent.radius, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Intensity", &lightComponent.intensity, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Inner Angle", &lightComponent.innerAngle, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Outer Angle", &lightComponent.outerAngle, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags) |
                GUI::InputFloat("Cone Falloff", &lightComponent.coneFalloff, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags);
            ImGui::SetCurrentContext(nullptr);
            return changed;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
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

        // Plugin::Component
        void save(Components::DirectionalLight const * const data, JSON::Object &componentData) const
        {
            componentData.set("intensity", data->intensity);
        }

        void load(Components::DirectionalLight * const data, JSON::Reference componentData)
        {
            data->intensity = parse(componentData.get("intensity"), 0.0f);
        }

        // Edit::Component
        bool ui(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data, uint32_t flags)
        {
            ImGui::SetCurrentContext(guiContext);
            auto &lightComponent = *dynamic_cast<Components::DirectionalLight *>(data);
            bool changed = 
                GUI::InputFloat("Intensity", &lightComponent.intensity, (flags & ImGuiInputTextFlags_ReadOnly ? -1.0f : 1.0f), 10.0f, 3, flags);
            ImGui::SetCurrentContext(nullptr);
            return false;
        }

        void show(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            ui(guiContext, entity, data, ImGuiInputTextFlags_ReadOnly);
        }

        bool edit(ImGuiContext * const guiContext, Math::Float4x4 const &viewMatrix, Math::Float4x4 const &projectionMatrix, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            return ui(guiContext, entity, data, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek