#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    interface Plugin
    {
        void initialize(IUnknown *initializerContext, LPCWSTR fileName);

        void enable(VideoContext *context);
    };

    DECLARE_INTERFACE_IID(PluginRegistration, "379FF0E3-85F2-4138-A6E4-BCA93241D4F0");
}; // namespace Gek
