/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Engine\Component.hpp"
#include "GEK\Shapes\Frustum.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(PointLight)
        {
            float range;
            float radius;
            float intensity;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };

        GEK_COMPONENT(SpotLight)
        {
            float range;
            float radius;
            float intensity;
            float innerAngle;
            float outerAngle;
            float falloff;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };

        GEK_COMPONENT(DirectionalLight)
        {
            float intensity;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Components
}; // namespace Gek
