#include "GEK/Math/Vector3.hpp"

namespace Gek
{
    namespace Math
    {
        template <>
        const Float3 Float3::Zero(0.0f, 0.0f, 0.0f);

        template <>
        const Float3 Float3::One(1.0f, 1.0f, 1.0f);

        template <>
        const Int3 Int3::Zero(0, 0, 0);

        template <>
        const Int3 Int3::One(1, 1, 1);

        template <>
        const UInt3 UInt3::Zero(0U, 0U, 0U);

        template <>
        const UInt3 UInt3::One(1U, 1U, 1U);
    }; // namespace Math
}; // namespace Gek
