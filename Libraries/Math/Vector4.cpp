#include "GEK/Math/Vector4.hpp"

namespace Gek
{
    namespace Math
    {
        template <>
        const Float4 Float4::Zero(0.0f, 0.0f, 0.0f, 0.0f);

        template <>
        const Float4 Float4::One(1.0f, 1.0f, 1.0f, 1.0f);

        template <>
        const Float4 Float4::Black(0.0f, 0.0f, 0.0f, 0.0f);

        template <>
        const Float4 Float4::White(1.0f, 1.0f, 1.0f, 1.0f);

        template <>
        const Int4 Int4::Zero(0, 0, 0, 0);

        template <>
        const Int4 Int4::One(1, 1, 1, 1);

        template <>
        const UInt4 UInt4::Zero(0U, 0U, 0U, 0U);

        template <>
        const UInt4 UInt4::One(1U, 1U, 1U, 1U);
    }; // namespace Math
}; // namespace Gek
