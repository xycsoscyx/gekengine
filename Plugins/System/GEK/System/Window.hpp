/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Utility/Context.hpp"
#include <wink/signal.hpp>

namespace Gek
{
    GEK_INTERFACE(Window)
    {
        enum class Cursor : uint8_t
        {
            None = 0,
            Arrow,
            Text,
            Hand,
            SizeNS,
            SizeEW,
            SizeNESW,
            SizeNWSE,
        };

        enum class Button : uint8_t
        {
            Left = 0,
            Middle,
            Right,
        };

        struct Description
        {
            std::string className;
            bool hasOwnContext = true;
            bool readMouseMovement = true;
            bool allowResize = false;

            std::string windowName;
            uint32_t initialWidth = 1;
            uint32_t initialHeight = 1;
        };

        virtual ~Window(void) = default;
        
        wink::signal<wink::slot<void(void)>> onClose;
        wink::signal<wink::slot<void(bool isActive)>> onActivate;
        wink::signal<wink::slot<void(bool isMinimized)>> onSizeChanged;

        wink::signal<wink::slot<void(uint32_t keyCode, bool state)>> onKeyPressed;
        wink::signal<wink::slot<void(uint32_t character)>> onCharacter;

        wink::signal<wink::slot<void(Cursor &cursor)>> onSetCursor;
        wink::signal<wink::slot<void(Button button, bool state)>> onMouseClicked;
        wink::signal<wink::slot<void(float numberOfRotations)>> onMouseWheel;
        wink::signal<wink::slot<void(int32_t xPosition, int32_t yPosition)>> onMousePosition;
        wink::signal<wink::slot<void(int32_t xMovement, int32_t yMovement)>> onMouseMovement;

        virtual void readEvents(void) = 0;

        virtual void *getBaseWindow(void) const = 0;

        virtual Math::Int4 getClientRectangle(bool moveToScreen = false) const = 0;
        virtual Math::Int4 getScreenRectangle(void) const = 0;

        virtual Math::Int2 getCursorPosition(void) const = 0;
        virtual void setCursorPosition(Math::Int2 const &position) = 0;
        virtual void setCursorVisibility(bool isVisible) = 0;

        virtual void setVisibility(bool isVisible) = 0;
        virtual void move(Math::Int2 const &position = Math::Int2(-1, -1)) = 0;
    };
}; // namespace Gek
