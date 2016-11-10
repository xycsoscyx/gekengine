/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include <limits>
#include <cstdint>

namespace Gek
{
	namespace Math
	{
		const float Infinity = std::numeric_limits<float>::max();
		const float NegativeInfinity = std::numeric_limits<float>::lowest();
        const float Epsilon = 1.0e-6f;
        const float Pi = 3.14159265358979323846f;
        const float Tau = (2.0f * Pi);
        const float E = 2.71828182845904523536f;
    }; // namespace Math
}; // namespace Gek
