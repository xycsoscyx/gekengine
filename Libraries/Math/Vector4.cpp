#include "GEK/Math/Vector4.hpp"

namespace Gek
{
    namespace Math
    {
        const Float4 Float4::Zero = Float4(0.0f, 0.0f, 0.0f, 0.0f);
        const Float4 Float4::One = Float4(1.0f, 1.0f, 1.0f, 1.0f);
        const Float4 Float4::Black = Float4(0.0f, 0.0f, 0.0f, 0.0f);
        const Float4 Float4::White = Float4(1.0f, 1.0f, 1.0f, 1.0f);
        const Int4 Int4::Zero = Int4(0, 0, 0, 0);
        const Int4 Int4::One = Int4(1, 1, 1, 1);
        const UInt4 UInt4::Zero = UInt4(0U, 0U, 0U, 0U);
        const UInt4 UInt4::One = UInt4(1U, 1U, 1U, 1U);
    }; // namespace Math
}; // namespace Gek
