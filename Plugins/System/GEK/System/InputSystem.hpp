/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace Input
    {
        GEK_ADD_EXCEPTION(CreationFailed);
        GEK_ADD_EXCEPTION(InitializationFailed);

        enum class Key : uint8_t
        {
            Escape              = 0x01,
            Number1             = 0x02,
            Number2             = 0x03,
            Number3             = 0x04,
            Number4             = 0x05,
            Number5             = 0x06,
            Number6             = 0x07,
            Number7             = 0x08,
            Number8             = 0x09,
            Number9             = 0x0A,
            Number0             = 0x0B,
            Minus               = 0x0C,    /* - on main keyboard */
            Equals              = 0x0D,
            Backspace           = 0x0E,    /* backspace */
            Tab                 = 0x0F,
            Q                   = 0x10,
            W                   = 0x11,
            E                   = 0x12,
            R                   = 0x13,
            T                   = 0x14,
            Y                   = 0x15,
            U                   = 0x16,
            I                   = 0x17,
            O                   = 0x18,
            P                   = 0x19,
            LeftBracket         = 0x1A,
            RightBracket        = 0x1B,
            Return              = 0x1C,    /* Enter on main keyboard */
            LeftControl         = 0x1D,
            A                   = 0x1E,
            S                   = 0x1F,
            D                   = 0x20,
            F                   = 0x21,
            G                   = 0x22,
            H                   = 0x23,
            J                   = 0x24,
            K                   = 0x25,
            L                   = 0x26,
            SemiColon           = 0x27,
            Apostrophe          = 0x28,
            Grave               = 0x29,    /* accent grave */
            LeftShift           = 0x2A,
            Backslash           = 0x2B,
            Z                   = 0x2C,
            X                   = 0x2D,
            C                   = 0x2E,
            V                   = 0x2F,
            B                   = 0x30,
            N                   = 0x31,
            M                   = 0x32,
            Comma               = 0x33,
            Period              = 0x34,    /* . on main keyboard */
            Slash               = 0x35,    /* / on main keyboard */
            RightShift          = 0x36,
            Multiply            = 0x37,    /* * on numeric keypad */
            LeftAlt             = 0x38,    /* left Alt */
            Space               = 0x39,
            CapsLock            = 0x3A,
            F1                  = 0x3B,
            F2                  = 0x3C,
            F3                  = 0x3D,
            F4                  = 0x3E,
            F5                  = 0x3F,
            F6                  = 0x40,
            F7                  = 0x41,
            F8                  = 0x42,
            F9                  = 0x43,
            F10                 = 0x44,
            NumberLock          = 0x45,
            ScrollLock          = 0x46,    /* Scroll Lock */
            NumberPad7          = 0x47,
            NumberPad8          = 0x48,
            NumberPad9          = 0x49,
            NumberPadSubtract   = 0x4A,    /* - on numeric keypad */
            NumberPad4          = 0x4B,
            NumberPad5          = 0x4C,
            NumberPad6          = 0x4D,
            NumberPadAdd        = 0x4E,    /* + on numeric keypad */
            NumberPad1          = 0x4F,
            NumberPad2          = 0x50,
            NumberPad3          = 0x51,
            NumberPad0          = 0x52,
            NumberPadDecimal    = 0x53,    /* . on numeric keypad */
            OEM102              = 0x56,    /* <> or \| on RT 102-key keyboard (Non-U.S.) */
            F11                 = 0x57,
            F12                 = 0x58,
            F13                 = 0x64,    /*                     (NEC PC98) */
            F14                 = 0x65,    /*                     (NEC PC98) */
            F15                 = 0x66,    /*                     (NEC PC98) */
            Kana                = 0x70,    /* (Japanese keyboard)            */
            ABNT_C1             = 0x73,    /* /? on Brazilian keyboard */
            Convert             = 0x79,    /* (Japanese keyboard)            */
            NoConvert           = 0x7B,    /* (Japanese keyboard)            */
            Yen                 = 0x7D,    /* (Japanese keyboard)            */
            ABNT_C2             = 0x7E,    /* Numpad . on Brazilian keyboard */
            NumberPadEquals     = 0x8D,    /*     = on numeric keypad (NEC PC98) */
            PreviousTrack       = 0x90,    /* Previous Track (CIRCUMFLEX on Japanese keyboard) */
            At                  = 0x91,    /*                     (NEC PC98) */
            Colon               = 0x92,    /*                     (NEC PC98) */
            Underline           = 0x93,    /*                     (NEC PC98) */
            Kanji               = 0x94,    /* (Japanese keyboard)            */
            Stop                = 0x95,    /*                     (NEC PC98) */
            Ax                  = 0x96,    /*                     (Japan AX) */
            Unlabeled           = 0x97,    /*                        (J3100) */
            NextTrack           = 0x99,    /* Next Track */
            NumberPadEnter      = 0x9C,    /* Enter on numeric keypad */
            RightControl        = 0x9D,
            Mute                = 0xA0,    /* Mute */
            Calculator          = 0xA1,    /* Calculator */
            PlayPause           = 0xA2,    /* Play / Pause */
            MediaStop           = 0xA4,    /* Media Stop */
            VolumeDown          = 0xAE,    /* Volume - */
            VolumeUp            = 0xB0,    /* Volume + */
            WebHome             = 0xB2,    /* Web home */
            NumberPadComma      = 0xB3,    /* , on numeric keypad (NEC PC98) */
            NumberPadDivide     = 0xB5,    /* / on numeric keypad */
            SystemRequest       = 0xB7,
            RightAlt            = 0xB8,    /* right Alt */
            Pause               = 0xC5,    /* Pause */
            Home                = 0xC7,    /* Home on arrow keypad */
            UpArrow             = 0xC8,    /* UpArrow on arrow keypad */
            PageuP              = 0xC9,    /* PgUp on arrow keypad */
            LeftArrow           = 0xCB,    /* LeftArrow on arrow keypad */
            RightArrow          = 0xCD,    /* RightArrow on arrow keypad */
            End                 = 0xCF,    /* End on arrow keypad */
            DownArrow           = 0xD0,    /* DownArrow on arrow keypad */
            PageDown            = 0xD1,    /* PgDn on arrow keypad */
            Insert              = 0xD2,    /* Insert on arrow keypad */
            Delete              = 0xD3,    /* Delete on arrow keypad */
            LeftWindows         = 0xDB,    /* Left Windows key */
            RightWindows        = 0xDC,    /* Right Windows key */
            Applications        = 0xDD,    /* AppMenu key */
            Power               = 0xDE,    /* System Power */
            Sleep               = 0xDF,    /* System Sleep */
            Wake                = 0xE3,    /* System Wake */
            WebSearch           = 0xE5,    /* Web Search */
            WebFavorites        = 0xE6,    /* Web Favorites */
            WebRefresh          = 0xE7,    /* Web Refresh */
            WebStop             = 0xE8,    /* Web Stop */
            WebForward          = 0xE9,    /* Web Forward */
            WebBackward         = 0xEA,    /* Web Back */
            NyComputer          = 0xEB,    /* My Computer */
            Mail                = 0xEC,    /* Mail */
            MediaSelect         = 0xED,    /* Media Select */
        };

        namespace State
        {
            enum
            {
                None            = 1 << 0,
                Released        = 1 << 1,
                Down            = 1 << 2,
                Pressed         = 1 << 3,
            };
        }; // namespace State

        GEK_INTERFACE(Device)
        {
            virtual ~Device(void) = default;

            virtual void poll(void) = 0;

            virtual uint32_t getButtonCount(void) const = 0;
            virtual uint8_t getButtonState(uint32_t buttonIndex) const = 0;

            virtual Math::Float3 getAxis(void) const = 0;
            virtual Math::Float3 getRotation(void) const = 0;
            virtual float getPointOfView(void) const = 0;
        };

        GEK_INTERFACE(System)
        {
            virtual ~System(void) = default;

            virtual Device * const getKeyboard(void) = 0;
            virtual Device * const getMouse(void) = 0;

            virtual uint32_t getJoystickCount(void) = 0;
            virtual Device * const getJoystick(uint32_t deviceIndex) = 0;

            virtual void pollAllDevices(void) = 0;
        };
    }; // namespace Input
}; // namespace Gek
