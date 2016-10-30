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
		const float Epsilon = std::numeric_limits<float>::min();
		const float Pi = 3.14159265358979323846f;

		template <typename TYPE>
		TYPE convertDegreesToRadians(TYPE degrees)
		{
			return TYPE(degrees * (Pi / 180.0f));
		}

		template <typename TYPE>
		TYPE convertRadiansToDegrees(TYPE radians)
		{
			return TYPE(radians * (180.0f / Pi));
		}

		template <typename DATA, typename TYPE>
		DATA lerp(const DATA &valueA, const DATA &valueB, TYPE factor)
		{
			return (((valueB - valueA) * factor) + valueA);
		}

		template <typename DATA, typename TYPE>
		DATA blend(const DATA &valueA, const DATA &valueB, TYPE factor)
		{
			return ((valueA * (1.0f - factor)) + (valueB * factor));
		}

		template <typename DATA, typename TYPE>
		DATA blend(const DATA &valueA, TYPE factorX, const DATA &valueB, TYPE factorY)
		{
			return ((valueA * factorX) + (valueB * factorY));
		}
    }; // namespace Math
}; // namespace Gek
