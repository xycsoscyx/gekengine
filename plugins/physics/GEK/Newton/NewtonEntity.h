#pragma once

#include "GEK\Engine\Entity.h"
#include <Newton.h>

namespace Gek
{
    GEK_INTERFACE(NewtonEntity)
    {
        virtual Entity * const getEntity(void) const = 0;

        virtual NewtonBody * const getNewtonBody(void) const = 0;

        virtual uint32_t getSurface(const Math::Float3 &position, const Math::Float3 &normal) = 0;

        // Called before the update phase to set the frame data for the body
        // Applies to rigid and player bodies
        virtual void onPreUpdate(float frameTime, int threadHandle) { };

        // Called after the update phase to react to changes in the world
        // Applies to player bodies only
        virtual void onPostUpdate(float frameTime, int threadHandle) { };

        // Called when setting the transformation matrix of the body
        // Applies to rigid and player bodies
        virtual void onSetTransform(const float* const matrixData, int threadHandle) { };
    };
}; // namespace Gek
