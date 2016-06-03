#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Context\Observable.h"

namespace Gek
{
    GEK_INTERFACE(NewtonProcessor)
        : virtual public Observable
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

        virtual uint32_t loadSurface(const wchar_t *fileName) = 0;
        virtual const Surface &getSurface(uint32_t surfaceIndex) const = 0;
    };

    GEK_INTERFACE(NewtonObserver)
        : virtual public Observer
    {
        virtual void onCollision(Entity *entity0, Entity *entity1, const Math::Float3 &position, const Math::Float3 &normal) { };
    };
}; // namespace Gek
