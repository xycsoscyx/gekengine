/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Engine/Core.hpp"
#include <Windows.h>

namespace Gek
{
    GEK_INTERFACE(Application)
    {
        struct Message
        {
            uint32_t identifier;
            WPARAM wParam;
            LPARAM lParam;

            Message(uint32_t identifier, WPARAM wParam, LPARAM lParam)
                : identifier(identifier)
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

        virtual Result sendMessage(const Message &message) = 0;

        virtual bool update(void) = 0;
    };
}; // namespace Gek
