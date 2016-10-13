/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Component.hpp"

namespace Gek
{
    namespace Private
    {
        GEK_COMPONENT(EditorCamera)
        {
            float headingAngle = 0.0f;
            float moveForward = 0.0f;
            float moveBackward = 0.0f;
            float strafeLeft = 0.0f;
            float strafeRight = 0.0f;
            float crouching = 0.0f;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);
        };
    }; // namespace Private
}; // namespace Gek
