#pragma once

#include "GEK\Engine\Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Filter)
        {
            std::vector<String> list;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Components
}; // namespace Gek
