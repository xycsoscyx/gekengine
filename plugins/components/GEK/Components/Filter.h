#pragma once

#include "GEK\Engine\Population.h"

namespace Gek
{
    namespace Components
    {
        struct Filter
        {
            std::vector<String> list;

            Filter(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
