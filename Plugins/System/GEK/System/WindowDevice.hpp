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
    namespace Window
    {
        enum class Key : uint8_t
        {
            Unknown = 0,
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
            Return = 40,
            Escape = 41,
            Backspace = 42,
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
            Delete = 76,
            Home = 74,
            End = 77,
            PageUp = 75,
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
            LeftControl = 224,
            LeftShift = 225,
            LeftAlt = 226,
            RightControl = 228,
            RightShift = 229,
            RightAlt = 230,
        };

        enum class Button : uint8_t
        {
            Left = 0,
            Middle,
            Right,
            Forward,
            Back,
            Unknown,
        };

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

        GEK_INTERFACE(Device)
        {
            virtual ~Device(void) = default;

            wink::signal<wink::slot<void(void)>> onCreated;
            wink::signal<wink::slot<void(void)>> onCloseRequested;
            wink::signal<wink::slot<void(void)>> onIdle;

            wink::signal<wink::slot<void(bool isActive)>> onActivate;
            wink::signal<wink::slot<void(bool isMinimized)>> onSizeChanged;

            wink::signal<wink::slot<void(Key keyCode, bool state)>> onKeyPressed;
            wink::signal<wink::slot<void(uint32_t character)>> onCharacter;

            wink::signal<wink::slot<void(Button button, bool state)>> onMouseClicked;
            wink::signal<wink::slot<void(float numberOfRotations)>> onMouseWheel;
            wink::signal<wink::slot<void(int32_t xPosition, int32_t yPosition)>> onMousePosition;
            wink::signal<wink::slot<void(int32_t xMovement, int32_t yMovement)>> onMouseMovement;

            virtual void create(Description const& description) = 0;
            virtual void close(void) = 0;

            virtual void* getWindowData(uint32_t data) const = 0;

            virtual Math::Int4 getClientRectangle(bool moveToScreen = false) const = 0;
            virtual Math::Int4 getScreenRectangle(void) const = 0;

            virtual Math::Int2 getCursorPosition(void) const = 0;
            virtual void setCursorPosition(Math::Int2 const& position) = 0;
            virtual void setCursorVisibility(bool isVisible) = 0;

            virtual void setVisibility(bool isVisible) = 0;
            virtual void move(Math::Int2 const& position = Math::Int2(-1, -1)) = 0;
            virtual void resize(Math::Int2 const& size) { };
        };
    }; // namespace Window
}; // namespace Gek
