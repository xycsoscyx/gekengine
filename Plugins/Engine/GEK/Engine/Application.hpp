/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Engine\Core.hpp"
#include <Windows.h>

namespace Gek
{
    GEK_INTERFACE(Application)
    {
        struct Event
        {
            uint32_t message;
            WPARAM wParam;
            LPARAM lParam;

            Event(uint32_t message, WPARAM wParam, LPARAM lParam)
                : message(message)
                , wParam(wParam)
                , lParam(lParam)
            {
            }
        };

        struct Result
        {
            bool handled = false;
            LRESULT result;

            Result(void)
            {
            }

            Result(LRESULT result)
                : handled(true)
                , result(result)
            {
            }
        };

        virtual ~Application(void) = default;

        virtual Result windowEvent(const Event &eventData) = 0;

        virtual bool update(void) = 0;
    };
}; // namespace Gek
