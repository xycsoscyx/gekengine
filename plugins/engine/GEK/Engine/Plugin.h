#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    GEK_INTERFACE(Plugin)
    {
        virtual void initialize(IUnknown *initializerContext, const wchar_t *fileName) = 0;

        virtual void enable(VideoContext *context) = 0;
    };
}; // namespace Gek
