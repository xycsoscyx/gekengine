#include "GEK/Shapes/Frustum.hpp"

namespace Gek
{
    namespace Shapes
    {
        Frustum::Frustum(void)
        {
        }

        Frustum::Frustum(const Math::Float4x4 &perspectiveTransform)
        {
            create(perspectiveTransform);
        }

        void Frustum::create(const Math::Float4x4 &perspectiveTransform)
        {
            // Left clipping plane
            planes[0].a = perspectiveTransform._14 + perspectiveTransform._11;
            planes[0].b = perspectiveTransform._24 + perspectiveTransform._21;
            planes[0].c = perspectiveTransform._34 + perspectiveTransform._31;
            planes[0].d = perspectiveTransform._44 + perspectiveTransform._41;
            
            // Right clipping plane
            planes[1].a = perspectiveTransform._14 - perspectiveTransform._11;
            planes[1].b = perspectiveTransform._24 - perspectiveTransform._21;
            planes[1].c = perspectiveTransform._34 - perspectiveTransform._31;
            planes[1].d = perspectiveTransform._44 - perspectiveTransform._41;
            
            // Bottom clipping plane
            planes[3].a = perspectiveTransform._14 + perspectiveTransform._12;
            planes[3].b = perspectiveTransform._24 + perspectiveTransform._22;
            planes[3].c = perspectiveTransform._34 + perspectiveTransform._32;
            planes[3].d = perspectiveTransform._44 + perspectiveTransform._42;

            // Top clipping plane
            planes[2].a = perspectiveTransform._14 - perspectiveTransform._12;
            planes[2].b = perspectiveTransform._24 - perspectiveTransform._22;
            planes[2].c = perspectiveTransform._34 - perspectiveTransform._32;
            planes[2].d = perspectiveTransform._44 - perspectiveTransform._42;
            
            // Near clipping plane
            planes[4].a = perspectiveTransform._13;
            planes[4].b = perspectiveTransform._23; 
            planes[4].c = perspectiveTransform._33;
            planes[4].d = perspectiveTransform._43;
            
            // Far clipping plane
            planes[5].a = perspectiveTransform._14 - perspectiveTransform._13;
            planes[5].b = perspectiveTransform._24 - perspectiveTransform._23;
            planes[5].c = perspectiveTransform._34 - perspectiveTransform._33;
            planes[5].d = perspectiveTransform._44 - perspectiveTransform._43;

            for (auto &plane : planes)
            {
                plane.normalize();
            }
        }
    }; // namespace Shapes
}; // namespace Gek
