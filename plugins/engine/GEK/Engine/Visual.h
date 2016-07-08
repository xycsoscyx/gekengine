#pragma once

#include "GEK\Context\Context.h"
#include "GEK\System\VideoDevice.h"

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Visual)
        {
            GEK_START_EXCEPTIONS();

            virtual void enable(Video::Device::Context *context) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
