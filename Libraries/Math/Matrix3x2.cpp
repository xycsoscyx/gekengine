#include "GEK/Math/Matrix3x2.hpp"

namespace Gek
{
    namespace Math
    {
        const Matrix3x2<float> Matrix3x2<float>::Identity = Matrix3x2<float>(
            1.0f, 0.0f,
            0.0f, 1.0f,
            0.0f, 0.0f);
    }; // namespace Math
}; // namespace Gek
