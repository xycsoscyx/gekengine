<<<<<<< master:libraries/math/Quaternion.cpp
#include "GEK\Math\Quaternion.hpp"
#include "GEK\Math\SIMD4x4.hpp"
#include "GEK\Math\Constants.hpp"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);
    }; // namespace Math
}; // namespace Gek
=======
#include "GEK\Math\Quaternion.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Math\Common.hpp"
#include <algorithm>

namespace Gek
{
    namespace Math
    {
        const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);
    }; // namespace Math
}; // namespace Gek
>>>>>>> local:libraries/math/SIMD/Quaternion.cpp
