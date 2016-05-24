#pragma once

#include <Windows.h>
#include "GEK\Math\Float3.h"

namespace Gek
{
    GEK_INTERFACE(NewtonProcessor)
    {
        struct Surface
        {
            bool ghost;
            float staticFriction;
            float kineticFriction;
            float elasticity;
            float softness;

            Surface(void)
                : ghost(false)
                , staticFriction(0.9f)
                , kineticFriction(0.5f)
                , elasticity(0.4f)
                , softness(1.0f)
            {
            }
        };

        virtual Math::Float3 getGravity(const Math::Float3 &position) = 0;

        virtual UINT32 loadSurface(const wchar_t *fileName) = 0;
        virtual const Surface &getSurface(UINT32 surfaceIndex) const = 0;
    };

    GEK_INTERFACE(NewtonObserver)
        : public Observer
    {
        virtual void onCollision(Entity *entity0, Entity *entity1, const Math::Float3 &position, const Math::Float3 &normal) = 0;
    };
}; // namespace Gek
