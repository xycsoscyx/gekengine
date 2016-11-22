/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Engine/Component.hpp"
#include "GEK/Shapes/Frustum.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(PointLight)
        {
            float range = 0.0f;
            float radius = 0.0f;
            float intensity = 0.0f;

            void save(JSON::Object &componentData) const;
            void load(const JSON::Object &componentData);
        };

        GEK_COMPONENT(SpotLight)
        {
            float range = 0.0f;
            float radius = 0.0f;
            float intensity = 0.0f;
            float innerAngle = 0.0f;
            float outerAngle = 0.0f;
            float coneFalloff = 0.0f;

            void save(JSON::Object &componentData) const;
            void load(const JSON::Object &componentData);
        };

        GEK_COMPONENT(DirectionalLight)
        {
            float intensity;

            void save(JSON::Object &componentData) const;
            void load(const JSON::Object &componentData);
        };
    }; // namespace Components
}; // namespace Gek
