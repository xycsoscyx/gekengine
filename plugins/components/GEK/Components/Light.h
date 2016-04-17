#pragma once

#include "GEK\Engine\Population.h"

namespace Gek
{
    struct PointLightComponent
    {
        float range;
        float radius;

        PointLightComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };

    struct SpotLightComponent
    {
        float range;
        float radius;
        float innerAngle;
        float outerAngle;

        SpotLightComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };

    struct DirectionalLightComponent
    {
        DirectionalLightComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
