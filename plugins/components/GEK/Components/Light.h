#pragma once

#include "GEK\Engine\Population.h"

namespace Gek
{
    struct PointLightComponent
    {
        float range;
        float radius;

        PointLightComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };

    struct SpotLightComponent
    {
        float range;
        float radius;
        float innerAngle;
        float outerAngle;
        float falloff;

        SpotLightComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };

    struct DirectionalLightComponent
    {
        DirectionalLightComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
