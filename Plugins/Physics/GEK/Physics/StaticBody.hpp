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
                SetMassMatrix(0.0f, shape);
                SetAutoSleep(true);
                SetNotifyCallback(nullptr); // No callback for static
            }
            ~StaticBody() override {}
            ndBody *getAsNewtonBody() override { return this; }
        };

        using StaticBodyPtr = std::unique_ptr<StaticBody>;
    } // namespace Physics
} // namespace Gek
