#include "GEK\Math\SIMD\Quaternion.hpp"

namespace Gek
{
    namespace Math
    {
        namespace SIMD
        {
            const Quaternion Quaternion::Identity = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
            const Quaternion Quaternion::Zero = _mm_setzero_ps();
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek
