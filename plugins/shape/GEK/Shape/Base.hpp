#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Engine\Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Shape)
        {
            String type;
            String parameters;
            String skin;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Components
}; // namespace Gek
