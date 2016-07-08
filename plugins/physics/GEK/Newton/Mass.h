#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    namespace Components
    {
        struct Mass
        {
            float value;

            Mass(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
