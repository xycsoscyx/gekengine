#pragma once

#include <cfloat>

namespace Gek
{
    namespace Math
    {
        const float Infinity = FLT_MAX;
        const float Epsilon = FLT_EPSILON;
        const float EpsilonSquared = (FLT_EPSILON * FLT_EPSILON);
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
        DATA lerp(const DATA &x, const DATA &y, TYPE factor)
        {
            return (((y - x) * factor) + x);
        }

        template <typename DATA, typename TYPE>
        DATA blend(const DATA &x, const DATA &y, TYPE factor)
        {
            return ((x * (1.0f - factor)) + (y * factor));
        }

        template <typename DATA, typename TYPE>
        DATA blend(const DATA &x, const DATA &y, TYPE factorX, TYPE factorY)
        {
            return ((x * factorX) + (y * factorY));
        }
    }; // namespace Math
}; // namespace Gek
