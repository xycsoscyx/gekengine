#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/API/ComponentMixin.hpp"
#include "GEK/API/Editor.hpp"
#include "GEK/Components/Light.hpp"

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
            LockedWrite{ std::cout } << "Range: " << data->range << ", Radius: " << data->radius << ", Intensity: " << data->intensity;
        }

        // Edit::Component
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &lightComponent = *dynamic_cast<Components::PointLight *>(data);

            changed |= editorElement("Range", [&](void) -> bool
            {
                return ImGui::InputFloat("##range", &lightComponent.range, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });
            
            changed |= editorElement("Radius", [&](void) -> bool
            {
                return ImGui::InputFloat("##radius", &lightComponent.radius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });
            
            changed |= editorElement("Intensity", [&](void) -> bool
            {
                return ImGui::InputFloat("##intensity", &lightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
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
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &lightComponent = *dynamic_cast<Components::SpotLight *>(data);

            changed |= editorElement("Range", [&](void) -> bool
            {
                return ImGui::InputFloat("##range", &lightComponent.range, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Radius", [&](void) -> bool
            {
                return ImGui::InputFloat("##radius", &lightComponent.radius, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Intensity", [&](void) -> bool
            {
                return ImGui::InputFloat("##intensity", &lightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            changed |= editorElement("Inner Angle", [&](void) -> bool
            {
                return ImGui::SliderAngle("##innerAngle", &lightComponent.innerAngle);
            });

            changed |= editorElement("Outer Angle", [&](void) -> bool
            {
                return ImGui::SliderAngle("##outerAngle", &lightComponent.outerAngle);
            });

            changed |= editorElement("Cone Falloff", [&](void) -> bool
            {
                return ImGui::InputFloat("##coneFalloff", &lightComponent.coneFalloff, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            ImGui::SetCurrentContext(nullptr);
            return changed;
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
        bool onUserInterface(ImGuiContext * const guiContext, Plugin::Entity * const entity, Plugin::Component::Data *data)
        {
            bool changed = false;
            ImGui::SetCurrentContext(guiContext);

            auto &lightComponent = *dynamic_cast<Components::DirectionalLight *>(data);

            changed |= editorElement("Intensity", [&](void) -> bool
            {
                return ImGui::InputFloat("##intensity", &lightComponent.intensity, 1.0f, 10.0f, 3, ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_CharsNoBlank);
            });

            ImGui::SetCurrentContext(nullptr);
            return false;
        }
    };

    GEK_REGISTER_CONTEXT_USER(PointLight);
    GEK_REGISTER_CONTEXT_USER(SpotLight);
    GEK_REGISTER_CONTEXT_USER(DirectionalLight);
}; // namespace Gek