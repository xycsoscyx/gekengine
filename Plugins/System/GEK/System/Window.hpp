/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/Context.hpp"
#include <nano_signal_slot.hpp>

namespace Gek
{
    GEK_INTERFACE(Window)
    {
        struct Event
        {
            struct Result
            {
                bool handled = false;
                int32_t result;

                Result(void)
                {
                }

                Result(int32_t result)
                    : handled(true)
                    , result(result)
                {
                }
            };

            uint32_t message;
            uint32_t unsignedParameter;
            int32_t signedParameter;

            Event(uint32_t message, uint32_t unsignedParameter, int32_t signedParameter)
                : message(message)
                , unsignedParameter(unsignedParameter)
                , signedParameter(signedParameter)
            {
            }
        };

        virtual ~Window(void) = default;

        std::function<Event::Result(const Event &eventData)> onEvent;
    };
}; // namespace Gek
