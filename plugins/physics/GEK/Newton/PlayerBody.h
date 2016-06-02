#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct PlayerBodyComponent
    {
        float height;
        float outerRadius;
        float innerRadius;
        float stairStep;

        PlayerBodyComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
