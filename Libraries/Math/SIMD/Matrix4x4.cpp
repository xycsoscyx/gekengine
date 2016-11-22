#include "GEK/Math/SIMD/Matrix4x4.hpp"

namespace Gek
{
    namespace Math
    {
        namespace SIMD
        {
            const Float4x4 Float4x4::Identity =
            {
                _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f),
                _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f),
                _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f),
                _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f)
            };
        }; // namespace SIMz
    }; // namespace Math
}; // namespace Gek
