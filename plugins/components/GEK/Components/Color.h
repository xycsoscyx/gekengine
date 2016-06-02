#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ColorComponent
    {
        Math::Color value;

        ColorComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
