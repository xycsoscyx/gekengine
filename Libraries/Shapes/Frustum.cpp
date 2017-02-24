#include "GEK/Shapes/Frustum.hpp"

namespace Gek
{
    namespace Shapes
    {
        Frustum::Frustum(void)
        {
        }

        Frustum::Frustum(const Frustum &frustum)
        {
            for (uint32_t index = 0; index < 6; index++)
            {
                planeList[index] = frustum.planeList[index];
            }
        }

        Frustum::Frustum(Math::Float4x4 const &perspectiveTransform)
        {
            create(perspectiveTransform);
        }

        void Frustum::create(Math::Float4x4 const &perspectiveTransform)
        {
            // Near clipping plane
            planeList[0].a = perspectiveTransform._13;
            planeList[0].b = perspectiveTransform._23;
            planeList[0].c = perspectiveTransform._33;
            planeList[0].d = perspectiveTransform._43;

            // Far clipping plane
            planeList[1].a = perspectiveTransform._14 - perspectiveTransform._13;
            planeList[1].b = perspectiveTransform._24 - perspectiveTransform._23;
            planeList[1].c = perspectiveTransform._34 - perspectiveTransform._33;
            planeList[1].d = perspectiveTransform._44 - perspectiveTransform._43;

            // Left clipping plane
            planeList[2].a = perspectiveTransform._14 + perspectiveTransform._11;
            planeList[2].b = perspectiveTransform._24 + perspectiveTransform._21;
            planeList[2].c = perspectiveTransform._34 + perspectiveTransform._31;
            planeList[2].d = perspectiveTransform._44 + perspectiveTransform._41;

            // Right clipping plane
            planeList[3].a = perspectiveTransform._14 - perspectiveTransform._11;
            planeList[3].b = perspectiveTransform._24 - perspectiveTransform._21;
            planeList[3].c = perspectiveTransform._34 - perspectiveTransform._31;
            planeList[3].d = perspectiveTransform._44 - perspectiveTransform._41;

            // Bottom clipping plane
            planeList[4].a = perspectiveTransform._14 + perspectiveTransform._12;
            planeList[4].b = perspectiveTransform._24 + perspectiveTransform._22;
            planeList[4].c = perspectiveTransform._34 + perspectiveTransform._32;
            planeList[4].d = perspectiveTransform._44 + perspectiveTransform._42;

            // Top clipping plane
            planeList[5].a = perspectiveTransform._14 - perspectiveTransform._12;
            planeList[5].b = perspectiveTransform._24 - perspectiveTransform._22;
            planeList[5].c = perspectiveTransform._34 - perspectiveTransform._32;
            planeList[5].d = perspectiveTransform._44 - perspectiveTransform._42;

            for (auto &plane : planeList)
            {
                plane.normalize();
            }
        }
    }; // namespace Shapes
}; // namespace Gek
