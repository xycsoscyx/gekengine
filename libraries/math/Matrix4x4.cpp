#include "GEK\Math\Matrix4x4.hpp"

namespace Gek
{
    namespace Math
    {
        const Float4x4 Float4x4::Identity(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
    }; // namespace Math
}; // namespace Gek
