#pragma once
#include "GEK/Physics/Base.hpp"
#include <dCollision/ndBodyKinematic.h>

namespace Gek
{
    namespace Physics
    {
        class StaticBody : public Body, public ndBodyKinematic
        {
          public:
            StaticBody(const Math::Float4x4 &matrix, ndShapeInstance shape)
            {
                SetMatrix(matrix.data);
                SetCollisionShape(shape);
                // Static bodies do not need inertia/mass computation.
                // On Linux this call can trigger deep shape processing for large BVHs
                // and has been observed to trip runtime overflow checks.
                SetAutoSleep(true);
                SetNotifyCallback(nullptr); // No callback for static
            }
            ~StaticBody() override {}
            ndBody *getAsNewtonBody() override { return this; }
        };

        using StaticBodyPtr = std::unique_ptr<StaticBody>;
    } // namespace Physics
} // namespace Gek
