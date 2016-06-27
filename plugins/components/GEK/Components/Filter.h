#pragma once

#include "GEK\Engine\Population.h"

namespace Gek
{
    struct FilterComponent
    {
        std::vector<String> list;

        FilterComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);
    };
}; // namespace Gek
