#pragma once

namespace Gek
{
    namespace Math
    {
        static union
        {
            unsigned char data[4];
            float value;
        } const HugeConstant = { { 0, 0, 0x80, 0x7f } };
        const float Infinity = HugeConstant.value;
        const float Epsilon = 1.0e-5f;
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
    }; // namespace Math
}; // namespace Gek
