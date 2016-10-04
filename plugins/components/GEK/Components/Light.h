#pragma once

#include "GEK\Engine\Component.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(PointLight)
        {
            float range;
            float radius;
            float intensity;

            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };

        GEK_COMPONENT(SpotLight)
        {
            float range;
            float radius;
            float intensity;
            float innerAngle;
            float outerAngle;
            float falloff;

            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };

        GEK_COMPONENT(DirectionalLight)
        {
            float intensity;

            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
