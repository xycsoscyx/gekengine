#include "API/System/WindowDevice.hpp"
#include "GEK/Utility/ContextUser.hpp"

#include <wayland-client.h>

#if __has_include(<xdg-shell-client-protocol.h>)
#include <xdg-shell-client-protocol.h>
#define GEK_WAYLAND_HAS_XDG_SHELL 1
#elif __has_include(<wayland-protocols/stable/xdg-shell/xdg-shell-client-protocol.h>)
#include <wayland-protocols/stable/xdg-shell/xdg-shell-client-protocol.h>
#define GEK_WAYLAND_HAS_XDG_SHELL 1
#else
#define GEK_WAYLAND_HAS_XDG_SHELL 0
#endif

#if __has_include(<pointer-constraints-unstable-v1-client-protocol.h>)
#include <pointer-constraints-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_POINTER_CONSTRAINTS 1
#elif __has_include(<wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1-client-protocol.h>)
#include <wayland-protocols/unstable/pointer-constraints/pointer-constraints-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_POINTER_CONSTRAINTS 1
#else
#define GEK_WAYLAND_HAS_POINTER_CONSTRAINTS 0
#endif

#if __has_include(<relative-pointer-unstable-v1-client-protocol.h>)
#include <relative-pointer-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_RELATIVE_POINTER 1
#elif __has_include(<wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1-client-protocol.h>)
#include <wayland-protocols/unstable/relative-pointer/relative-pointer-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_RELATIVE_POINTER 1
#else
#define GEK_WAYLAND_HAS_RELATIVE_POINTER 0
#endif

#if __has_include(<xdg-decoration-unstable-v1-client-protocol.h>)
#include <xdg-decoration-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_XDG_DECORATION 1
#elif __has_include(<wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1-client-protocol.h>)
#include <wayland-protocols/unstable/xdg-decoration/xdg-decoration-unstable-v1-client-protocol.h>
#define GEK_WAYLAND_HAS_XDG_DECORATION 1
#else
#define GEK_WAYLAND_HAS_XDG_DECORATION 0
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <unistd.h>

namespace Gek
{
    namespace Window::Implementation
    {
        static void traceWaylandEvent(char const *message)
        {
            if (message)
            {
                std::fprintf(stderr, "%s\n", message);
                std::fflush(stderr);
            }
        }

        static Window::Key convertLinuxKey(uint32_t key)
        {
            switch (key)
            {
            case 1:
                return Window::Key::Escape;
            case 2:
                return Window::Key::Key1;
            case 3:
                return Window::Key::Key2;
            case 4:
                return Window::Key::Key3;
            case 5:
                return Window::Key::Key4;
            case 6:
                return Window::Key::Key5;
            case 7:
                return Window::Key::Key6;
            case 8:
                return Window::Key::Key7;
            case 9:
                return Window::Key::Key8;
            case 10:
                return Window::Key::Key9;
            case 11:
                return Window::Key::Key0;
            case 14:
                return Window::Key::Backspace;
            case 15:
                return Window::Key::Tab;
            case 16:
                return Window::Key::Q;
            case 17:
                return Window::Key::W;
            case 18:
                return Window::Key::E;
            case 19:
                return Window::Key::R;
            case 20:
                return Window::Key::T;
            case 21:
                return Window::Key::Y;
            case 22:
                return Window::Key::U;
            case 23:
                return Window::Key::I;
            case 24:
                return Window::Key::O;
            case 25:
                return Window::Key::P;
            case 26:
                return Window::Key::LeftBracket;
            case 27:
                return Window::Key::RightBracket;
            case 28:
                return Window::Key::Return;
            case 29:
                return Window::Key::LeftControl;
            case 30:
                return Window::Key::A;
            case 31:
                return Window::Key::S;
            case 32:
                return Window::Key::D;
            case 33:
                return Window::Key::F;
            case 34:
                return Window::Key::G;
            case 35:
                return Window::Key::H;
            case 36:
                return Window::Key::J;
            case 37:
                return Window::Key::K;
            case 38:
                return Window::Key::L;
            case 39:
                return Window::Key::Semicolon;
            case 40:
                return Window::Key::Quote;
            case 41:
                return Window::Key::Grave;
            case 42:
                return Window::Key::LeftShift;
            case 43:
                return Window::Key::Backslash;
            case 44:
                return Window::Key::Z;
            case 45:
                return Window::Key::X;
            case 46:
                return Window::Key::C;
            case 47:
                return Window::Key::V;
            case 48:
                return Window::Key::B;
            case 49:
                return Window::Key::N;
            case 50:
                return Window::Key::M;
            case 51:
                return Window::Key::Comma;
            case 52:
                return Window::Key::Period;
            case 53:
                return Window::Key::Slash;
            case 54:
                return Window::Key::RightShift;
            case 55:
                return Window::Key::KeyPadMultiply;
            case 56:
                return Window::Key::LeftAlt;
            case 57:
                return Window::Key::Space;
            case 58:
                return Window::Key::CapsLock;
            case 59:
                return Window::Key::F1;
            case 60:
                return Window::Key::F2;
            case 61:
                return Window::Key::F3;
            case 62:
                return Window::Key::F4;
            case 63:
                return Window::Key::F5;
            case 64:
                return Window::Key::F6;
            case 65:
                return Window::Key::F7;
            case 66:
                return Window::Key::F8;
            case 67:
                return Window::Key::F9;
            case 68:
                return Window::Key::F10;
            case 69:
                return Window::Key::KeyPadNumLock;
            case 70:
                return Window::Key::ScrollLock;
            case 71:
                return Window::Key::KeyPad7;
            case 72:
                return Window::Key::KeyPad8;
            case 73:
                return Window::Key::KeyPad9;
            case 74:
                return Window::Key::KeyPadSubtract;
            case 75:
                return Window::Key::KeyPad4;
            case 76:
                return Window::Key::KeyPad5;
            case 77:
                return Window::Key::KeyPad6;
            case 78:
                return Window::Key::KeyPadAdd;
            case 79:
                return Window::Key::KeyPad1;
            case 80:
                return Window::Key::KeyPad2;
            case 81:
                return Window::Key::KeyPad3;
            case 82:
                return Window::Key::KeyPad0;
            case 83:
                return Window::Key::KeyPadPoint;
            case 87:
                return Window::Key::F11;
            case 88:
                return Window::Key::F12;
            case 97:
                return Window::Key::RightControl;
            case 100:
                return Window::Key::RightAlt;
            case 102:
                return Window::Key::Home;
            case 103:
                return Window::Key::Up;
            case 104:
                return Window::Key::PageUp;
            case 105:
                return Window::Key::Left;
            case 106:
                return Window::Key::Right;
            case 107:
                return Window::Key::End;
            case 108:
                return Window::Key::Down;
            case 109:
                return Window::Key::PageDown;
            case 110:
                return Window::Key::Insert;
            case 111:
                return Window::Key::Delete;
            default:
                return Window::Key::Unknown;
            }
        }

        GEK_CONTEXT_USER_BASE(Device)
        , public Window::Device
        {
          private:
            wl_display *display = nullptr;
            wl_registry *registry = nullptr;
            wl_compositor *compositor = nullptr;
            wl_seat *seat = nullptr;
            wl_pointer *pointer = nullptr;
            wl_keyboard *keyboard = nullptr;
            wl_surface *surface = nullptr;

#if GEK_WAYLAND_HAS_XDG_SHELL
            xdg_wm_base *wmBase = nullptr;
            xdg_surface *xdgSurface = nullptr;
            xdg_toplevel *xdgToplevel = nullptr;
#endif

#if GEK_WAYLAND_HAS_XDG_DECORATION
            zxdg_decoration_manager_v1 *decorationManager = nullptr;
            zxdg_toplevel_decoration_v1 *toplevelDecoration = nullptr;
#endif

#if GEK_WAYLAND_HAS_POINTER_CONSTRAINTS
            zwp_pointer_constraints_v1 *pointerConstraints = nullptr;
            zwp_locked_pointer_v1 *lockedPointer = nullptr;
#endif

#if GEK_WAYLAND_HAS_RELATIVE_POINTER
            zwp_relative_pointer_manager_v1 *relativePointerManager = nullptr;
            zwp_relative_pointer_v1 *relativePointer = nullptr;
#endif

            uint32_t clientWidth = 1;
            uint32_t clientHeight = 1;
            bool cursorVisible = true;
            bool readMouseMovement = true;
            bool isMinimized = false;
            bool hasReportedInitialSize = false;
            bool seatCapabilitiesReceived = false;
            uint32_t seatCapabilityMask = 0;
            Math::Int2 cursorPosition = Math::Int2::Zero;
            uint64_t pointerEnterCount = 0;
            uint64_t pointerMotionCount = 0;
            uint64_t pointerButtonCount = 0;
            uint64_t keyboardKeyCount = 0;
            std::chrono::steady_clock::time_point lastHeartbeat = std::chrono::steady_clock::now();

            std::atomic_bool stop = false;

          public:
            Device(Context * context)
                : ContextRegistration(context)
            {
            }

            ~Device(void)
            {
#if GEK_WAYLAND_HAS_RELATIVE_POINTER
                if (relativePointer)
                {
                    zwp_relative_pointer_v1_destroy(relativePointer);
                    relativePointer = nullptr;
                }

                if (relativePointerManager)
                {
                    zwp_relative_pointer_manager_v1_destroy(relativePointerManager);
                    relativePointerManager = nullptr;
                }
#endif

#if GEK_WAYLAND_HAS_XDG_DECORATION
                if (toplevelDecoration)
                {
                    zxdg_toplevel_decoration_v1_destroy(toplevelDecoration);
                    toplevelDecoration = nullptr;
                }

                if (decorationManager)
                {
                    zxdg_decoration_manager_v1_destroy(decorationManager);
                    decorationManager = nullptr;
                }
#endif

#if GEK_WAYLAND_HAS_POINTER_CONSTRAINTS
                if (lockedPointer)
                {
                    zwp_locked_pointer_v1_destroy(lockedPointer);
                    lockedPointer = nullptr;
                }

                if (pointerConstraints)
                {
                    zwp_pointer_constraints_v1_destroy(pointerConstraints);
                    pointerConstraints = nullptr;
                }
#endif

#if GEK_WAYLAND_HAS_XDG_SHELL
                if (xdgToplevel)
                {
                    xdg_toplevel_destroy(xdgToplevel);
                    xdgToplevel = nullptr;
                }

                if (xdgSurface)
                {
                    xdg_surface_destroy(xdgSurface);
                    xdgSurface = nullptr;
                }

                if (wmBase)
                {
                    xdg_wm_base_destroy(wmBase);
                    wmBase = nullptr;
                }
#endif

                if (keyboard)
                {
                    wl_keyboard_destroy(keyboard);
                    keyboard = nullptr;
                }

                if (pointer)
                {
                    wl_pointer_destroy(pointer);
                    pointer = nullptr;
                }

                if (seat)
                {
                    wl_seat_destroy(seat);
                    seat = nullptr;
                }

                if (surface)
                {
                    wl_surface_destroy(surface);
                    surface = nullptr;
                }

                if (compositor)
                {
                    wl_compositor_destroy(compositor);
                    compositor = nullptr;
                }

                if (registry)
                {
                    wl_registry_destroy(registry);
                    registry = nullptr;
                }

                if (display)
                {
                    wl_display_disconnect(display);
                    display = nullptr;
                }
            }

          private:
            static void registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interfaceName, uint32_t version)
            {
                auto *device = reinterpret_cast<Device *>(data);

                if (std::strcmp(interfaceName, wl_compositor_interface.name) == 0)
                {
                    device->compositor = reinterpret_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 6u)));
                    return;
                }

                if (std::strcmp(interfaceName, wl_seat_interface.name) == 0)
                {
                    device->seat = reinterpret_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 7u)));
                    return;
                }

#if GEK_WAYLAND_HAS_XDG_SHELL
                if (std::strcmp(interfaceName, xdg_wm_base_interface.name) == 0)
                {
                    device->wmBase = reinterpret_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 6u)));
                    return;
                }
#endif

#if GEK_WAYLAND_HAS_POINTER_CONSTRAINTS
                if (std::strcmp(interfaceName, zwp_pointer_constraints_v1_interface.name) == 0)
                {
                    device->pointerConstraints = reinterpret_cast<zwp_pointer_constraints_v1 *>(wl_registry_bind(registry, name, &zwp_pointer_constraints_v1_interface, 1));
                    return;
                }
#endif

#if GEK_WAYLAND_HAS_RELATIVE_POINTER
                if (std::strcmp(interfaceName, zwp_relative_pointer_manager_v1_interface.name) == 0)
                {
                    device->relativePointerManager = reinterpret_cast<zwp_relative_pointer_manager_v1 *>(wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, 1));
                    return;
                }
#endif

#if GEK_WAYLAND_HAS_XDG_DECORATION
                if (std::strcmp(interfaceName, zxdg_decoration_manager_v1_interface.name) == 0)
                {
                    device->decorationManager = reinterpret_cast<zxdg_decoration_manager_v1 *>(wl_registry_bind(registry, name, &zxdg_decoration_manager_v1_interface, 1));
                    return;
                }
#endif
            }

            static void registryGlobalRemove(void *data, wl_registry *registry, uint32_t name)
            {
                (void)data;
                (void)registry;
                (void)name;
            }

#if GEK_WAYLAND_HAS_XDG_SHELL
            static void wmBasePing(void *data, xdg_wm_base *wmBase, uint32_t serial)
            {
                (void)data;
                xdg_wm_base_pong(wmBase, serial);
            }

            static void xdgSurfaceConfigure(void *data, xdg_surface *xdgSurface, uint32_t serial)
            {
                auto *device = reinterpret_cast<Device *>(data);
                xdg_surface_ack_configure(xdgSurface, serial);
                device->getContext()->log(Context::Debug, "Wayland xdg_surface configure ack: serial={}, size={}x{}", serial, device->clientWidth, device->clientHeight);

                if (!device->hasReportedInitialSize)
                {
                    device->hasReportedInitialSize = true;
                    device->onSizeChanged(device->isMinimized);
                }

                if (device->surface)
                {
                    wl_surface_commit(device->surface);
                }
            }

            static void xdgTopLevelConfigure(void *data, xdg_toplevel *xdgToplevel, int32_t width, int32_t height, wl_array *states)
            {
                (void)xdgToplevel;
                auto *device = reinterpret_cast<Device *>(data);

                bool minimized = false;
                if (states && states->data && states->size >= sizeof(uint32_t))
                {
                    const uint32_t *stateList = reinterpret_cast<const uint32_t *>(states->data);
                    const uint32_t count = static_cast<uint32_t>(states->size / sizeof(uint32_t));
                    for (uint32_t index = 0; index < count; ++index)
                    {
                        if (stateList[index] == XDG_TOPLEVEL_STATE_ACTIVATED)
                        {
                            device->onActivate(true);
                        }

                        if (stateList[index] == XDG_TOPLEVEL_STATE_SUSPENDED)
                        {
                            minimized = true;
                        }
                    }
                }

                if (width > 0)
                {
                    device->clientWidth = static_cast<uint32_t>(width);
                }

                if (height > 0)
                {
                    device->clientHeight = static_cast<uint32_t>(height);
                }

                if ((width > 0) || (height > 0) || (device->isMinimized != minimized))
                {
                    device->isMinimized = minimized;
                    device->onSizeChanged(device->isMinimized);
                }

                device->getContext()->log(Context::Debug, "Wayland xdg_toplevel configure: width={}, height={}, applied={}x{}, minimized={}",
                                          width,
                                          height,
                                          device->clientWidth,
                                          device->clientHeight,
                                          device->isMinimized);
            }

            static void xdgTopLevelClose(void *data, xdg_toplevel *xdgToplevel)
            {
                (void)xdgToplevel;
                auto *device = reinterpret_cast<Device *>(data);
                device->onCloseRequested();
                device->stop.store(true);
            }
#endif

            static void seatCapabilities(void *data, wl_seat *seat, uint32_t capabilities)
            {
                auto *device = reinterpret_cast<Device *>(data);
                (void)seat;

                device->seatCapabilitiesReceived = true;
                device->seatCapabilityMask = capabilities;
                device->getContext()->log(Context::Info, "Wayland seat capabilities: pointer={}, keyboard={}, touch={}",
                                          ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0),
                                          ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0),
                                          ((capabilities & WL_SEAT_CAPABILITY_TOUCH) != 0));

                if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0)
                {
                    if (!device->pointer)
                    {
                        device->pointer = wl_seat_get_pointer(seat);
                        if (device->pointer)
                        {
                            static const wl_pointer_listener pointerListener = {
                                pointerEnter,
                                pointerLeave,
                                pointerMotion,
                                pointerButton,
                                pointerAxis,
                            };

                            wl_pointer_add_listener(device->pointer, &pointerListener, device);
                            device->updatePointerExtensions();
                        }
                    }
                }
                else if (device->pointer)
                {
                    wl_pointer_destroy(device->pointer);
                    device->pointer = nullptr;
                }

                if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0)
                {
                    if (!device->keyboard)
                    {
                        device->keyboard = wl_seat_get_keyboard(seat);
                        if (device->keyboard)
                        {
                            static const wl_keyboard_listener keyboardListener = {
                                keyboardKeymap,
                                keyboardEnter,
                                keyboardLeave,
                                keyboardKey,
                                keyboardModifiers,
                            };

                            wl_keyboard_add_listener(device->keyboard, &keyboardListener, device);
                        }
                    }
                }
                else if (device->keyboard)
                {
                    wl_keyboard_destroy(device->keyboard);
                    device->keyboard = nullptr;
                }
            }

            static void seatName(void *data, wl_seat *seat, const char *name)
            {
                (void)data;
                (void)seat;
                (void)name;
            }

            static void pointerEnter(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface, wl_fixed_t x, wl_fixed_t y)
            {
                (void)pointer;
                (void)serial;
                (void)surface;

                auto *device = reinterpret_cast<Device *>(data);
                ++device->pointerEnterCount;
                device->cursorPosition.x = wl_fixed_to_int(x);
                device->cursorPosition.y = wl_fixed_to_int(y);
                device->getContext()->log(Context::Debug, "Wayland pointer enter: x={}, y={}", device->cursorPosition.x, device->cursorPosition.y);
                device->onActivate(true);
                device->onMousePosition(device->cursorPosition.x, device->cursorPosition.y);
            }

            static void pointerLeave(void *data, wl_pointer *pointer, uint32_t serial, wl_surface *surface)
            {
                (void)data;
                (void)pointer;
                (void)serial;
                (void)surface;
            }

            static void pointerMotion(void *data, wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y)
            {
                (void)pointer;
                (void)time;

                auto *device = reinterpret_cast<Device *>(data);
                ++device->pointerMotionCount;
                device->cursorPosition.x = wl_fixed_to_int(x);
                device->cursorPosition.y = wl_fixed_to_int(y);
                device->onMousePosition(device->cursorPosition.x, device->cursorPosition.y);
            }

            static void pointerButton(void *data, wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
            {
                (void)pointer;
                (void)time;

                auto *device = reinterpret_cast<Device *>(data);
                ++device->pointerButtonCount;
                auto buttonState = (state == WL_POINTER_BUTTON_STATE_PRESSED);
                device->getContext()->log(Context::Debug, "Wayland pointer button: serial={}, code=0x{:x}, pressed={}", serial, button, buttonState);

                switch (button)
                {
                case 0x110:
                    device->onMouseClicked(Window::Button::Left, buttonState);
                    break;
                case 0x111:
                    device->onMouseClicked(Window::Button::Right, buttonState);
                    break;
                case 0x112:
                    device->onMouseClicked(Window::Button::Middle, buttonState);
                    break;
                case 0x115:
                    device->onMouseClicked(Window::Button::Forward, buttonState);
                    break;
                case 0x116:
                    device->onMouseClicked(Window::Button::Back, buttonState);
                    break;
                default:
                    break;
                }
            }

            static void pointerAxis(void *data, wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
            {
                (void)pointer;
                (void)time;
                (void)axis;

                auto *device = reinterpret_cast<Device *>(data);
                device->onMouseWheel(-static_cast<float>(wl_fixed_to_double(value)));
            }

            static void keyboardKeymap(void *data, wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size)
            {
                (void)data;
                (void)keyboard;
                (void)format;
                (void)size;

                if (fd >= 0)
                {
                    ::close(fd);
                }
            }

            static void keyboardEnter(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface, wl_array *keys)
            {
                (void)keyboard;
                (void)serial;
                (void)surface;
                (void)keys;

                auto *device = reinterpret_cast<Device *>(data);
                device->onActivate(true);
            }

            static void keyboardLeave(void *data, wl_keyboard *keyboard, uint32_t serial, wl_surface *surface)
            {
                (void)keyboard;
                (void)serial;
                (void)surface;

                auto *device = reinterpret_cast<Device *>(data);
                device->onActivate(false);
            }

            static void keyboardKey(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
            {
                (void)keyboard;
                (void)serial;
                (void)time;

                auto *device = reinterpret_cast<Device *>(data);
                ++device->keyboardKeyCount;
                auto keyState = (state == WL_KEYBOARD_KEY_STATE_PRESSED);
                device->getContext()->log(Context::Debug, "Wayland keyboard key: code={}, pressed={}", key, keyState);
                device->onKeyPressed(convertLinuxKey(key), keyState);
            }

            static void keyboardModifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
            {
                (void)data;
                (void)keyboard;
                (void)serial;
                (void)depressed;
                (void)latched;
                (void)locked;
                (void)group;
            }

#if GEK_WAYLAND_HAS_RELATIVE_POINTER
            static void relativePointerMotion(
                void *data,
                zwp_relative_pointer_v1 *relativePointer,
                uint32_t utime_hi,
                uint32_t utime_lo,
                wl_fixed_t dx,
                wl_fixed_t dy,
                wl_fixed_t dx_unaccel,
                wl_fixed_t dy_unaccel)
            {
                (void)relativePointer;
                (void)utime_hi;
                (void)utime_lo;
                (void)dx;
                (void)dy;

                auto *device = reinterpret_cast<Device *>(data);
                device->onMouseMovement(wl_fixed_to_int(dx_unaccel), wl_fixed_to_int(dy_unaccel));
            }
#endif

#if GEK_WAYLAND_HAS_POINTER_CONSTRAINTS
            static void lockedPointerLocked(void *data, zwp_locked_pointer_v1 *lockedPointer)
            {
                (void)data;
                (void)lockedPointer;
            }

            static void lockedPointerUnlocked(void *data, zwp_locked_pointer_v1 *lockedPointer)
            {
                auto *device = reinterpret_cast<Device *>(data);
                if (device->lockedPointer == lockedPointer)
                {
                    zwp_locked_pointer_v1_destroy(device->lockedPointer);
                    device->lockedPointer = nullptr;
                }
            }
#endif

            void updatePointerExtensions(void)
            {
#if GEK_WAYLAND_HAS_RELATIVE_POINTER
                if (readMouseMovement && pointer && relativePointerManager && !relativePointer)
                {
                    relativePointer = zwp_relative_pointer_manager_v1_get_relative_pointer(relativePointerManager, pointer);
                    if (relativePointer)
                    {
                        static const zwp_relative_pointer_v1_listener relativePointerListener = {
                            relativePointerMotion,
                        };

                        zwp_relative_pointer_v1_add_listener(relativePointer, &relativePointerListener, this);
                    }
                }
#endif

#if GEK_WAYLAND_HAS_POINTER_CONSTRAINTS
                if (readMouseMovement && !cursorVisible && pointerConstraints && pointer && surface && !lockedPointer)
                {
                    lockedPointer = zwp_pointer_constraints_v1_lock_pointer(
                        pointerConstraints,
                        surface,
                        pointer,
                        nullptr,
                        ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

                    if (lockedPointer)
                    {
                        static const zwp_locked_pointer_v1_listener lockedPointerListener = {
                            lockedPointerLocked,
                            lockedPointerUnlocked,
                        };

                        zwp_locked_pointer_v1_add_listener(lockedPointer, &lockedPointerListener, this);
                    }
                }
                else if ((cursorVisible || !readMouseMovement) && lockedPointer)
                {
                    zwp_locked_pointer_v1_destroy(lockedPointer);
                    lockedPointer = nullptr;
                }
#endif
            }

          public:
            void create(Window::Description const &description)
            {
#if !GEK_WAYLAND_HAS_XDG_SHELL
                (void)description;
                throw std::runtime_error("Wayland xdg-shell headers are required to build the Wayland window plugin");
#else
                clientWidth = std::max<uint32_t>(1, description.initialWidth);
                clientHeight = std::max<uint32_t>(1, description.initialHeight);
                readMouseMovement = description.readMouseMovement;
                getContext()->log(Context::Info, "Wayland initial window size request: {}x{}", clientWidth, clientHeight);

                display = wl_display_connect(nullptr);
                if (!display)
                {
                    throw std::runtime_error("Unable to connect to Wayland display");
                }

                registry = wl_display_get_registry(display);
                if (!registry)
                {
                    throw std::runtime_error("Unable to get Wayland registry");
                }

                static const wl_registry_listener registryListener = {
                    registryGlobal,
                    registryGlobalRemove,
                };
                wl_registry_add_listener(registry, &registryListener, this);

                if (wl_display_roundtrip(display) < 0 || wl_display_roundtrip(display) < 0)
                {
                    throw std::runtime_error("Unable to initialize required Wayland globals");
                }

                if (!compositor || !wmBase)
                {
                    throw std::runtime_error("Wayland compositor or xdg_wm_base is unavailable");
                }

                if (seat)
                {
                    static const wl_seat_listener seatListener = {
                        seatCapabilities,
                        seatName,
                    };

                    wl_seat_add_listener(seat, &seatListener, this);

                    // Seat capability events can be emitted before listener registration.
                    // Perform one sync pass here to guarantee pointer/keyboard objects are created.
                    if (wl_display_roundtrip(display) < 0)
                    {
                        throw std::runtime_error("Unable to synchronize Wayland seat capabilities");
                    }

                    if (!seatCapabilitiesReceived)
                    {
                        getContext()->log(Context::Warning, "Wayland seat listener did not receive capabilities after sync");
                    }
                    else
                    {
                        getContext()->log(Context::Info, "Wayland seat synchronized: pointer={}, keyboard={}",
                                          ((seatCapabilityMask & WL_SEAT_CAPABILITY_POINTER) != 0),
                                          ((seatCapabilityMask & WL_SEAT_CAPABILITY_KEYBOARD) != 0));
                        traceWaylandEvent("Wayland seat synchronized");
                    }
                }
                else
                {
                    getContext()->log(Context::Warning, "Wayland compositor did not expose wl_seat; input is unavailable");
                    traceWaylandEvent("Wayland seat unavailable");
                }

                static const xdg_wm_base_listener wmBaseListener = {
                    wmBasePing,
                };
                xdg_wm_base_add_listener(wmBase, &wmBaseListener, this);

                surface = wl_compositor_create_surface(compositor);
                if (!surface)
                {
                    throw std::runtime_error("Unable to create Wayland surface");
                }

                xdgSurface = xdg_wm_base_get_xdg_surface(wmBase, surface);
                if (!xdgSurface)
                {
                    throw std::runtime_error("Unable to create xdg_surface");
                }

                static const xdg_surface_listener xdgSurfaceListener = {
                    xdgSurfaceConfigure,
                };
                xdg_surface_add_listener(xdgSurface, &xdgSurfaceListener, this);

                xdgToplevel = xdg_surface_get_toplevel(xdgSurface);
                if (!xdgToplevel)
                {
                    throw std::runtime_error("Unable to create xdg_toplevel");
                }

                static const xdg_toplevel_listener xdgToplevelListener = {
                    xdgTopLevelConfigure,
                    xdgTopLevelClose,
                };
                xdg_toplevel_add_listener(xdgToplevel, &xdgToplevelListener, this);

                if (!description.windowName.empty())
                {
                    xdg_toplevel_set_title(xdgToplevel, description.windowName.c_str());
                }

                if (!description.className.empty())
                {
                    xdg_toplevel_set_app_id(xdgToplevel, description.className.c_str());
                }

#if GEK_WAYLAND_HAS_XDG_DECORATION
                if (decorationManager)
                {
                    toplevelDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(decorationManager, xdgToplevel);
                    if (toplevelDecoration)
                    {
                        zxdg_toplevel_decoration_v1_set_mode(toplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
                    }
                }
#endif

                xdg_toplevel_set_min_size(xdgToplevel, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));
                xdg_surface_set_window_geometry(xdgSurface, 0, 0, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));

                wl_surface_commit(surface);
                wl_display_roundtrip(display);

                getContext()->log(Context::Info, "Wayland surface committed; initial negotiated size: {}x{}", clientWidth, clientHeight);

                updatePointerExtensions();

                try
                {
                    onCreated();
                }
                catch (std::exception const &exception)
                {
                    getContext()->log(Context::Error, "WaylandDevice onCreated exception: {}", exception.what());
                    stop.store(true);
                }
                catch (...)
                {
                    getContext()->log(Context::Error, "WaylandDevice onCreated unknown exception");
                    stop.store(true);
                }

                uint32_t consecutiveIdleExceptions = 0;
                while (!stop.load())
                {
                    auto now = std::chrono::steady_clock::now();
                    if ((now - lastHeartbeat) >= std::chrono::seconds(1))
                    {
                        lastHeartbeat = now;
                        getContext()->log(
                            Context::Warning,
                            "Wayland heartbeat: seat={}, pointer={}, keyboard={}, enter={}, motion={}, button={}, key={}",
                            (seat != nullptr),
                            (pointer != nullptr),
                            (keyboard != nullptr),
                            pointerEnterCount,
                            pointerMotionCount,
                            pointerButtonCount,
                            keyboardKeyCount);

                        char heartbeat[256] = {};
                        std::snprintf(
                            heartbeat,
                            sizeof(heartbeat),
                            "Wayland heartbeat seat=%d pointer=%d keyboard=%d enter=%llu motion=%llu button=%llu key=%llu",
                            (seat != nullptr) ? 1 : 0,
                            (pointer != nullptr) ? 1 : 0,
                            (keyboard != nullptr) ? 1 : 0,
                            static_cast<unsigned long long>(pointerEnterCount),
                            static_cast<unsigned long long>(pointerMotionCount),
                            static_cast<unsigned long long>(pointerButtonCount),
                            static_cast<unsigned long long>(keyboardKeyCount));
                        traceWaylandEvent(heartbeat);
                    }

                    if (wl_display_dispatch_pending(display) < 0)
                    {
                        stop.store(true);
                        break;
                    }

                    auto displayFD = wl_display_get_fd(display);
                    if (displayFD >= 0)
                    {
                        pollfd descriptor = { displayFD, POLLIN, 0 };
                        auto pollResult = ::poll(&descriptor, 1, 0);
                        if ((pollResult > 0) && (descriptor.revents & POLLIN))
                        {
                            if (wl_display_dispatch(display) < 0)
                            {
                                stop.store(true);
                                break;
                            }
                        }
                        else
                        {
                            wl_display_flush(display);
                        }
                    }

                    try
                    {
                        onIdle();
                        consecutiveIdleExceptions = 0;
                    }
                    catch (std::exception const &exception)
                    {
                        const auto *systemError = dynamic_cast<std::system_error const *>(&exception);
                        const bool transientBusy =
                            (systemError != nullptr) &&
                            (systemError->code() == std::make_error_code(std::errc::device_or_resource_busy));

                        if (transientBusy)
                        {
                            getContext()->log(Context::Warning, "WaylandDevice onIdle transient exception (continuing): {}", exception.what());
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                            continue;
                        }

                        getContext()->log(Context::Error, "WaylandDevice onIdle exception: {}", exception.what());
                        if (++consecutiveIdleExceptions >= 8)
                        {
                            stop.store(true);
                        }
                    }
                    catch (...)
                    {
                        getContext()->log(Context::Error, "WaylandDevice onIdle unknown exception");
                        if (++consecutiveIdleExceptions >= 8)
                        {
                            stop.store(true);
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
#endif
            }

            void close(void)
            {
                stop.store(true);
            }

            void *getWindowData(uint32_t data) const
            {
                switch (data)
                {
                case 0:
                    return display;
                case 1:
                    return surface;
                default:
                    return nullptr;
                }
            }

            Math::Int4 getClientRectangle(bool moveToScreen = false) const
            {
                (void)moveToScreen;
                return Math::Int4(0, 0, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));
            }

            Math::Int4 getScreenRectangle(void) const
            {
                return Math::Int4(0, 0, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));
            }

            Math::Int2 getCursorPosition(void) const
            {
                return cursorPosition;
            }

            void setCursorPosition(Math::Int2 const &position)
            {
                (void)position;
                getContext()->log(Context::Debug, "Wayland does not support globally warping the cursor position");
            }

            void setCursorVisibility(bool isVisible)
            {
                cursorVisible = isVisible;
                updatePointerExtensions();
            }

            void setVisibility(bool isVisible)
            {
                (void)isVisible;
            }

            void move(Math::Int2 const &position)
            {
                (void)position;
            }

            void resize(Math::Int2 const &size)
            {
                clientWidth = static_cast<uint32_t>(std::max<int32_t>(1, size.x));
                clientHeight = static_cast<uint32_t>(std::max<int32_t>(1, size.y));

                getContext()->log(Context::Info, "Wayland resize requested: {}x{}", clientWidth, clientHeight);

#if GEK_WAYLAND_HAS_XDG_SHELL
                if (xdgToplevel)
                {
                    xdg_toplevel_set_min_size(xdgToplevel, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));
                }

                if (xdgSurface)
                {
                    xdg_surface_set_window_geometry(xdgSurface, 0, 0, static_cast<int32_t>(clientWidth), static_cast<int32_t>(clientHeight));
                }
#endif

                if (surface)
                {
                    wl_surface_commit(surface);
                }

                onSizeChanged(isMinimized);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // namespace Window::Implementation
}; // namespace Gek
