#include "GEK\Math\Color.hpp"
#include "GEK\Math\Float3.hpp"
#include "GEK\Math\Float4.hpp"
#include "GEK\Math\Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Color Color::Black(0.0f, 0.0f, 0.0f, 0.0f);
        const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);

        Color::Color(const Float3 &vector)
            : r(vector.x)
            , g(vector.y)
            , b(vector.z)
            , a(1.0f)
        {
        }

        Color::Color(const Float4 &vector)
            : r(vector.x)
            , g(vector.y)
            , b(vector.z)
            , a(vector.w)
        {
        }

        Float3 Color::getXYZ(void) const
        {
            return Float3(r, g, b);
        }

        Float4 Color::getXYZW(void) const
        {
            return Float4(r, g, b, a);
        }

        void Color::set(const Float3 &vector)
        {
            r = vector.x;
            g = vector.y;
            b = vector.z;
            a = 1.0f;
        }

        void Color::set(const Float4 &vector)
        {
            r = vector.x;
            g = vector.y;
            b = vector.z;
            a = vector.w;
        }

        float Color::dot(const Color &color) const
        {
            return ((r * color.r) + (g * color.g) + (b * color.b) + (a * color.a));
        }

        Color Color::lerp(const Color &color, float factor) const
        {
            return Math::lerp((*this), color, factor);
        }

    }; // namespace Math
}; // namespace Gek
