#pragma once

#include "GEK\Context\Context.hpp"
#include "GEK\System\VideoDevice.hpp"

namespace Gek
{
    namespace Plugin
    {
        GEK_INTERFACE(Visual)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(InvalidElementType);
            GEK_ADD_EXCEPTION(MissingParameters);

            virtual void enable(Video::Device::Context *context) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
