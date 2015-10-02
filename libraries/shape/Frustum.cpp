#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Shape
    {
        Frustum::Frustum(const Math::Float4x4 &transform)
        {
            // Calculate near plane of frustum.
            planes[0].normal.x = transform._14 + transform._13;
            planes[0].normal.y = transform._24 + transform._23;
            planes[0].normal.z = transform._34 + transform._33;
            planes[0].distance = transform._44 + transform._43;
            planes[0].normalize();

            // Calculate far plane of frustum.
            planes[1].normal.x = transform._14 - transform._13;
            planes[1].normal.y = transform._24 - transform._23;
            planes[1].normal.z = transform._34 - transform._33;
            planes[1].distance = transform._44 - transform._43;
            planes[1].normalize();

            // Calculate left plane of frustum.
            planes[2].normal.x = transform._14 + transform._11;
            planes[2].normal.y = transform._24 + transform._21;
            planes[2].normal.z = transform._34 + transform._31;
            planes[2].distance = transform._44 + transform._41;
            planes[2].normalize();

            // Calculate right plane of frustum.
            planes[3].normal.x = transform._14 - transform._11;
            planes[3].normal.y = transform._24 - transform._21;
            planes[3].normal.z = transform._34 - transform._31;
            planes[3].distance = transform._44 - transform._41;
            planes[3].normalize();

            // Calculate top plane of frustum.
            planes[4].normal.x = transform._14 - transform._12;
            planes[4].normal.y = transform._24 - transform._22;
            planes[4].normal.z = transform._34 - transform._32;
            planes[4].distance = transform._44 - transform._42;
            planes[4].normalize();

            // Calculate bottom plane of frustum.
            planes[5].normal.x = transform._14 + transform._12;
            planes[5].normal.y = transform._24 + transform._22;
            planes[5].normal.z = transform._34 + transform._32;
            planes[5].distance = transform._44 + transform._42;
            planes[5].normalize();
        }
    }; // namespace Shape
}; // namespace Gek
