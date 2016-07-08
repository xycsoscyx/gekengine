#pragma once

#include "GEK\Engine\Population.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    namespace Components
    {
        struct PointLight
        {
            float range;
            float radius;

            PointLight(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };

        struct SpotLight
        {
            float range;
            float radius;
            float innerAngle;
            float outerAngle;
            float falloff;

            SpotLight(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };

        struct DirectionalLight
        {
            DirectionalLight(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
