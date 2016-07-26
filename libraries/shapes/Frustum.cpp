#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    namespace Shapes
    {
        Frustum::Frustum(const Math::Float4x4 &transform)
        {
            Math::Float4x4 transpose(transform.getTranspose());

            // Calculate left plane of frustum.
            planes[2].vector = transpose[0] + transpose[3];

            // Calculate right plane of frustum.
            planes[3].vector = -transpose[0] + transpose[3];

            // Calculate bottom plane of frustum.
            planes[4].vector = transpose[1] + transpose[3];

            // Calculate top plane of frustum.
            planes[5].vector = -transpose[1] + transpose[3];

            // Calculate near plane of frustum.
            planes[0].vector = transpose[2] + transpose[3];

            // Calculate far plane of frustum.
            planes[1].vector = -transpose[2] + transpose[3];

            for (auto &plane : planes)
            {
                plane.normalize();
            }
        }
    }; // namespace Shapes
}; // namespace Gek
