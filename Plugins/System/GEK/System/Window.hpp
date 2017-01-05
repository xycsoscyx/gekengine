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
        struct Description
        {
            String className;
            bool hasOwnContext = true;

            String windowName;
            uint32_t initialWidth = 1;
            uint32_t initialHeight = 1;
            bool getRawInput = true;
        };

        struct Event
        {
            struct Result
            {
                bool handled = false;
                int32_t value = 0;;

                Result(void)
                {
                }

                Result(int32_t value)
                    : handled(true)
                    , value(value)
                {
                }
            } result;

            const uint32_t message;
            const uint32_t unsignedParameter;
            const int32_t signedParameter;

            Event(uint32_t message, uint32_t unsignedParameter, int32_t signedParameter)
                : message(message)
                , unsignedParameter(unsignedParameter)
                , signedParameter(signedParameter)
            {
            }
        };

        virtual ~Window(void) = default;

        Nano::Signal<void(Event &data)> onEvent;

        virtual void readEvents(void) = 0;
        virtual Event::Result sendEvent(const Event &eventData) = 0;

        virtual void *getPrivateData(void) const = 0;

        virtual Math::Int4 getClientRectangle(void) const = 0;
        virtual Math::Int4 getScreenRectangle(void) const = 0;

        virtual Math::Int2 getCursorPosition(void) const = 0;
        virtual void setCursorPosition(const Math::Int2 &position) = 0;

        virtual void move(int32_t xPosition = -1, int32_t yPosition = -1) = 0;
    };
}; // namespace Gek
