#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Model)
        {
            String name;
            String skin;

            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);
        };
    }; // namespace Components
}; // namespace Gek
