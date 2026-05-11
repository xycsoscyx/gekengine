#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Physics/Base.hpp"

namespace Gek
{
    namespace Physics
    {
        inline ndMatrix MakeNewtonMatrix(Math::Float4x4 const &matrix)
        {
            ndVector axisX(matrix.r.x.data);
            ndVector axisY(matrix.r.y.data);
            ndVector axisZ(matrix.r.z.data);
            ndVector axisW(matrix.r.w.data);
            return ndMatrix(axisX, axisY, axisZ, axisW);
        }
    } // namespace Physics
} // namespace Gek