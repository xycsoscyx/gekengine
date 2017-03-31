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
        enum class Button : uint32_t
        {
            Left = 0,
            Middle,
            Right,
        };

        struct Description
        {
            WString className;
            bool hasOwnContext = true;

            WString windowName;
            uint32_t initialWidth = 1;
            uint32_t initialHeight = 1;
            bool readMouseMovement = true;
        };

        virtual ~Window(void) = default;
        
        Nano::Signal<void(void)> onClose;
        Nano::Signal<void(bool isActive)> onActivate;
        Nano::Signal<void(bool isMinimized)> onSizeChanged;

        Nano::Signal<void(uint16_t key, bool state)> onKeyPressed;
        Nano::Signal<void(wchar_t character)> onCharacter;

        Nano::Signal<void(bool &showCursor)> onSetCursor;
        Nano::Signal<void(Button button, bool state)> onMouseClicked;
        Nano::Signal<void(int32_t offset)> onMouseWheel;
        Nano::Signal<void(int32_t xPosition, int32_t yPosition)> onMousePosition;
        Nano::Signal<void(int32_t xMovement, int32_t yMovement)> onMouseMovement;

        virtual void readEvents(void) = 0;

        virtual void *getBaseWindow(void) const = 0;

        virtual Math::Int4 getClientRectangle(void) const = 0;
        virtual Math::Int4 getScreenRectangle(void) const = 0;

        virtual Math::Int2 getCursorPosition(void) const = 0;
        virtual void setCursorPosition(Math::Int2 const &position) = 0;

        virtual void setVisibility(bool isVisible) = 0;
        virtual void move(Math::Int2 const &position = Math::Int2(-1, -1)) = 0;
    };
}; // namespace Gek
