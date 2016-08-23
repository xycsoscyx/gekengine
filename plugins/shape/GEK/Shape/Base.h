#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    namespace Components
    {
        struct Shape
        {
            String type;
            String parameters;
            String skin;

            Shape(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
