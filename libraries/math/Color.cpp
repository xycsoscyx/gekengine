#include "GEK\Math\Color.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Common.h"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        Color::Color(const Float4 &vector)
            : r(vector.x)
            , g(vector.y)
            , b(vector.z)
            , a(vector.w)
        {
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
