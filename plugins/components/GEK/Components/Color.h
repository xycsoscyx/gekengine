#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    namespace Components
    {
        struct Color
        {
            Math::Color value;

            Color(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
