#pragma once

#include <Windows.h>
#include "GEK\Engine\Entity.h"
#include <Newton.h>

namespace Gek
{
    GEK_INTERFACE(NewtonEntity)
    {
        Entity * const getEntity(void) const = 0;

        NewtonBody * const getNewtonBody(void) const = 0;

        UINT32 getSurface(const Math::Float3 &position, const Math::Float3 &normal) = 0;

        // Called before the update phase to set the frame data for the body
        // Applies to rigid and player bodies
        void onPreUpdate(float frameTime, int threadHandle) = default;

        // Called after the update phase to react to changes in the world
        // Applies to player bodies only
        void onPostUpdate(float frameTime, int threadHandle) = default;

        // Called when setting the transformation matrix of the body
        // Applies to rigid and player bodies
        void onSetTransform(const float* const matrixData, int threadHandle) = default;
    };
}; // namespace Gek
