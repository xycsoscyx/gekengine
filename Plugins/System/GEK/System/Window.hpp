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
#include <lsignal.h>

namespace Gek
{
    GEK_INTERFACE(Window)
    {
        enum class Key
        {
            A = 4,
            B = 5,
            C = 6,
            D = 7,
            E = 8,
            F = 9,
            G = 10,
            H = 11,
            I = 12,
            J = 13,
            K = 14,
            L = 15,
            M = 16,
            N = 17,
            O = 18,
            P = 19,
            Q = 20,
            R = 21,
            S = 22,
            T = 23,
            U = 24,
            V = 25,
            W = 26,
            X = 27,
            Y = 28,
            Z = 29,
            Key1 = 30,
            Key2 = 31,
            Key3 = 32,
            Key4 = 33,
            Key5 = 34,
            Key6 = 35,
            Key7 = 36,
            Key8 = 37,
            Key9 = 38,
            Key0 = 39,
            Enter = 40,
            Escape = 41,
            Delete = 42,
            Tab = 43,
            Space = 44,
            Minus = 45,
            Equals = 46,
            LeftBracket = 47,
            RightBracket = 48,
            Backslash = 49,
            Semicolon = 51,
            Quote = 52,
            Grave = 53,
            Comma = 54,
            Period = 55,
            Slash = 56,
            CapsLock = 57,
            F1 = 58,
            F2 = 59,
            F3 = 60,
            F4 = 61,
            F5 = 62,
            F6 = 63,
            F7 = 64,
            F8 = 65,
            F9 = 66,
            F10 = 67,
            F11 = 68,
            F12 = 69,
            PrintScreen = 70,
            ScrollLock = 71,
            Pause = 72,
            Insert = 73,
            Home = 74,
            PageUp = 75,
            DeleteForward = 76,
            End = 77,
            PageDown = 78,
            Right = 79,
            Left = 80,
            Down = 81,
            Up = 82,
            KeyPadNumLock = 83,
            KeyPadDivide = 84,
            KeyPadMultiply = 85,
            KeyPadSubtract = 86,
            KeyPadAdd = 87,
            KeyPadEnter = 88,
            KeyPad1 = 89,
            KeyPad2 = 90,
            KeyPad3 = 91,
            KeyPad4 = 92,
            KeyPad5 = 93,
            KeyPad6 = 94,
            KeyPad7 = 95,
            KeyPad8 = 96,
            KeyPad9 = 97,
            KeyPad0 = 98,
            KeyPadPoint = 99,
            NonUSBackslash = 100,
            KeyPadEquals = 103,
            F13 = 104,
            F14 = 105,
            F15 = 106,
            F16 = 107,
            F17 = 108,
            F18 = 109,
            F19 = 110,
            F20 = 111,
            F21 = 112,
            F22 = 113,
            F23 = 114,
            F24 = 115,
            Help = 117,
            Menu = 118,
            LeftControl = 224,
            LeftShift = 225,
            LeftAlt = 226,
            LeftGUI = 227,
            RightControl = 228,
            RightShift = 229,
            RightAlt = 230,
            RightGUI = 231
        };

        enum class Button : uint32_t
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
        
        lsignal::signal<void(void)> onClose;
        lsignal::signal<void(bool isActive)> onActivate;
        lsignal::signal<void(bool isMinimized)> onSizeChanged;

        lsignal::signal<void(Key key, bool state)> onKeyPressed;
        lsignal::signal<void(wchar_t character)> onCharacter;

        lsignal::signal<void(bool &showCursor)> onSetCursor;
        lsignal::signal<void(Button button, bool state)> onMouseClicked;
        lsignal::signal<void(float numberOfRotations)> onMouseWheel;
        lsignal::signal<void(int32_t xPosition, int32_t yPosition)> onMousePosition;
        lsignal::signal<void(int32_t xMovement, int32_t yMovement)> onMouseMovement;

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
