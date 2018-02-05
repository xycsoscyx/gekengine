/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/API/Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(PointLight)
        {
			GEK_COMPONENT_DATA(PointLight);

			float range = 0.0f;
            float radius = 0.0f;
            float intensity = 0.0f;
        };

        GEK_COMPONENT(SpotLight)
        {
			GEK_COMPONENT_DATA(SpotLight);

			float range = 0.0f;
            float radius = 0.0f;
            float intensity = 0.0f;
            float innerAngle = 0.0f;
            float outerAngle = 0.0f;
            float coneFalloff = 0.0f;
        };

        GEK_COMPONENT(DirectionalLight)
        {
			GEK_COMPONENT_DATA(DirectionalLight);

			float intensity;
        };
    }; // namespace Components
}; // namespace Gek
