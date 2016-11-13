#include "GEK\Math\SIMD\Vector4.hpp"

namespace Gek
{
    namespace Math
    {
		namespace SIMD
		{
            const Float4 Float4::Zero = _mm_setzero_ps();
            const Float4 Float4::Half = _mm_set1_ps(0.5f);
            const Float4 Float4::One = _mm_set1_ps(1.0f);
            const Float4 Float4::Two = _mm_set1_ps(2.0f);
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek
