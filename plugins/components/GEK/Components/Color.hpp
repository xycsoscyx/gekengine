#pragma once

#include "GEK\Math\Color.hpp"
#include "GEK\Engine\Component.hpp"

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
