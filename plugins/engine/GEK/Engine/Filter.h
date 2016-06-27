#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include <memory>

namespace Gek
{
    GEK_PREDECLARE(RenderPipeline);
    GEK_PREDECLARE(RenderContext);

    GEK_INTERFACE(Filter)
    {
        GEK_INTERFACE(Pass)
        {
            enum class Mode : uint8_t
            {
                Deferred = 0,
                Compute,
            };

            typedef std::unique_ptr<Pass> Iterator;

            virtual Iterator next(void) = 0;
            virtual Mode prepare(void) = 0;
        };

        virtual Pass::Iterator begin(RenderContext *renderContext, ResourceHandle cameraTarget) = 0;
    };
}; // namespace Gek
