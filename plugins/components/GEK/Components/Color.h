#pragma once

#include "GEK\Math\Color.h"
#include "GEK\Engine\Component.h"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Color)
        {
            Math::Color value;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Components
}; // namespace Gek
