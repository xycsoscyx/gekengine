/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include <algorithm>
#include <cstdint>
#include <climits>
#include <cmath>

namespace Gek
{
	namespace Math
	{
		const float Infinity = std::numeric_limits<float>::max();
		const float NegativeInfinity = std::numeric_limits<float>::lowest();
        const float NotANumber = std::nanf("Gek::Math");
		const float Epsilon = 1.0e-5f;
        const float Pi = 3.14159265358979323846f;
        const float Tau = (2.0f * Pi);
        const float E = 2.71828182845904523536f;

		template <typename TYPE>
		TYPE DegreesToRadians(TYPE degrees)
		{
			return TYPE(degrees * (Pi / 180.0f));
		}

		template <typename TYPE>
		TYPE RadiansToDegrees(TYPE radians)
		{
			return TYPE(radians * (180.0f / Pi));
		}

		template <typename DATA, typename TYPE>
		DATA Interpolate(const DATA &valueA, const DATA &valueB, TYPE factor)
		{
			return (((valueB - valueA) * factor) + valueA);
		}

		template <typename DATA, typename TYPE>
		DATA Blend(const DATA &valueA, const DATA &valueB, TYPE factor)
		{
			return ((valueA * (1.0f - factor)) + (valueB * factor));
		}

		template <typename DATA, typename TYPE>
		DATA Blend(const DATA &valueA, TYPE factorX, const DATA &valueB, TYPE factorY)
		{
			return ((valueA * factorX) + (valueB * factorY));
		}

        template <typename TYPE>
        TYPE Clamp(TYPE value, TYPE min, TYPE max)
        {
            return std::min(std::max(value, min), max);
        }
    }; // namespace Math
}; // namespace Gek
