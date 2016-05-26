#pragma once

#include "GEK\Context\Context.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    GEK_INTERFACE(Plugin)
    {
        virtual void enable(VideoContext *context) = 0;
    };
}; // namespace Gek
