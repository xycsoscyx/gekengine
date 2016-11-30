#include "GEK/Math/Vector2.hpp"

namespace Gek
{
    namespace Math
    {
        template <>
        const Float2 Float2::Zero(0.0f, 0.0f);

        template <>
        const Float2 Float2::One(1.0f, 1.0f);

        template <>
        const Int2 Int2::Zero(0, 0);

        template <>
        const Int2 Int2::One(1, 1);

        template <>
        const UInt2 UInt2::Zero(0U, 0U);

        template <>
        const UInt2 UInt2::One(1U, 1U);
    }; // namespace Math
}; // namespace Gek
