#pragma once

#include "GEK\Context\Context.hpp"
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
