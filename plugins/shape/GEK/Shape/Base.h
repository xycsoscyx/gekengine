#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Engine\Component.h"

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
