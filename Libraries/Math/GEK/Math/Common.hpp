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
		static constexpr float Infinity = std::numeric_limits<float>::max();
		static constexpr float NegativeInfinity = std::numeric_limits<float>::lowest();
		static const float NotANumber = std::nanf("Gek::Math");
		static constexpr float Epsilon = 1.0e-5f;
		static constexpr float Pi = 3.14159265358979323846f;
		static constexpr float Tau = (2.0f * Pi);
		static constexpr float E = 2.71828182845904523536f;

		template <typename TYPE>
		TYPE DegreesToRadians(TYPE const degrees) noexcept
		{
			return TYPE(degrees * (Pi / 180.0f));
		}

		template <typename TYPE>
		TYPE RadiansToDegrees(TYPE const radians) noexcept
		{
			return TYPE(radians * (180.0f / Pi));
		}

		template <typename DATA, typename TYPE>
		DATA Interpolate(DATA const &valueA, DATA const &valueB, TYPE const factor) noexcept
		{
			return (((valueB - valueA) * factor) + valueA);
		}

		template <typename DATA, typename TYPE>
		DATA Blend(DATA const &valueA, DATA const &valueB, TYPE const factor) noexcept
		{
			return ((valueA * (1.0f - factor)) + (valueB * factor));
		}

		template <typename DATA, typename TYPE>
		DATA Blend(DATA const &valueA, TYPE const factorX, DATA const &valueB, TYPE const factorY) noexcept
		{
			return ((valueA * factorX) + (valueB * factorY));
		}

        template <typename TYPE>
        TYPE Clamp(TYPE const value, TYPE const min, TYPE const max) noexcept
        {
            return std::min(std::max(value, min), max);
        }
    }; // namespace Math
}; // namespace Gek
