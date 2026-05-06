cmake_minimum_required(VERSION 3.15)
project(systemwayland)

file(GLOB SOURCES "entrypoints.cpp" "WaylandDevice.cpp")

add_library(systemwayland SHARED ${SOURCES})
set_target_properties(systemwayland PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    PREFIX ""
    OUTPUT_NAME $<IF:$<CONFIG:Debug>,systemwayland_debug,systemwayland>
    DEBUG_POSTFIX ""
)

target_link_libraries(systemwayland PUBLIC Math Utility Resources Common)

# --- Find wayland-client (no pkg-config dependency) ---
# Try CMake's built-in FindWayland first (CMake >= 3.22).
find_package(Wayland QUIET COMPONENTS Client)
if(Wayland_FOUND)
    target_link_libraries(systemwayland PRIVATE Wayland::Client)
else()
    # Direct library/header search — works without pkg-config as long as libwayland-dev is installed.
    find_library(WAYLAND_CLIENT_LIB NAMES wayland-client
        REQUIRED
        DOC "wayland-client library — install libwayland-dev")
    find_path(WAYLAND_CLIENT_INCLUDE_DIR NAMES wayland-client.h
        REQUIRED
        DOC "wayland-client headers — install libwayland-dev")
    if(NOT WAYLAND_CLIENT_LIB OR NOT WAYLAND_CLIENT_INCLUDE_DIR)
        message(FATAL_ERROR
            "wayland-client not found.\n"
            "Install libwayland-dev (Debian/Ubuntu: sudo apt-get install -y libwayland-dev)")
    endif()
    target_include_directories(systemwayland PRIVATE "${WAYLAND_CLIENT_INCLUDE_DIR}")
    target_link_libraries(systemwayland PRIVATE "${WAYLAND_CLIENT_LIB}")
endif()

# --- Find wayland-protocols data directory ---
# Try pkg-config first; fall back to the fixed install path on Debian/Ubuntu.
set(WAYLAND_PROTOCOLS_DIR "")
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(WAYLAND_PROTOCOLS QUIET wayland-protocols)
    if(WAYLAND_PROTOCOLS_FOUND)
        pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)
    endif()
endif()
if(NOT WAYLAND_PROTOCOLS_DIR OR NOT EXISTS "${WAYLAND_PROTOCOLS_DIR}")
    if(EXISTS "/usr/share/wayland-protocols")
        set(WAYLAND_PROTOCOLS_DIR "/usr/share/wayland-protocols")
    else()
        message(FATAL_ERROR
            "wayland-protocols data directory not found.\n"
            "Install wayland-protocols (Debian/Ubuntu: sudo apt-get install -y wayland-protocols)")
    endif()
endif()
message(STATUS "wayland-protocols dir: ${WAYLAND_PROTOCOLS_DIR}")

# --- Find wayland-scanner ---
find_program(WAYLAND_SCANNER wayland-scanner
    HINTS /usr/bin /usr/local/bin
    DOC "wayland-scanner — install libwayland-bin")
if(NOT WAYLAND_SCANNER)
    message(FATAL_ERROR
        "wayland-scanner not found.\n"
        "Install libwayland-bin (Debian/Ubuntu: sudo apt-get install -y libwayland-bin)")
endif()

set(WL_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/wayland-generated")
file(MAKE_DIRECTORY "${WL_GENERATED_DIR}")

# xdg-shell (stable)
set(XDG_SHELL_XML "${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml")
execute_process(COMMAND "${WAYLAND_SCANNER}" client-header "${XDG_SHELL_XML}" "${WL_GENERATED_DIR}/xdg-shell-client-protocol.h")
execute_process(COMMAND "${WAYLAND_SCANNER}" private-code  "${XDG_SHELL_XML}" "${WL_GENERATED_DIR}/xdg-shell-client-protocol.c")

# pointer-constraints (unstable v1)
set(POINTER_CONSTRAINTS_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml")
execute_process(COMMAND "${WAYLAND_SCANNER}" client-header "${POINTER_CONSTRAINTS_XML}" "${WL_GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.h")
execute_process(COMMAND "${WAYLAND_SCANNER}" private-code  "${POINTER_CONSTRAINTS_XML}" "${WL_GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.c")

# relative-pointer (unstable v1)
set(RELATIVE_POINTER_XML "${WAYLAND_PROTOCOLS_DIR}/unstable/relative-pointer/relative-pointer-unstable-v1.xml")
execute_process(COMMAND "${WAYLAND_SCANNER}" client-header "${RELATIVE_POINTER_XML}" "${WL_GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.h")
execute_process(COMMAND "${WAYLAND_SCANNER}" private-code  "${RELATIVE_POINTER_XML}" "${WL_GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.c")

target_sources(systemwayland PRIVATE
    "${WL_GENERATED_DIR}/xdg-shell-client-protocol.c"
    "${WL_GENERATED_DIR}/pointer-constraints-unstable-v1-client-protocol.c"
    "${WL_GENERATED_DIR}/relative-pointer-unstable-v1-client-protocol.c"
)

target_include_directories(systemwayland PRIVATE "${WL_GENERATED_DIR}")

if(UNIX AND NOT APPLE)
    set_target_properties(systemwayland PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../..:$ORIGIN/../../..:$ORIGIN/../../../lib"
    )
endif()

install(TARGETS systemwayland
    RUNTIME DESTINATION bin/plugins/system
    LIBRARY DESTINATION bin/plugins/system
    ARCHIVE DESTINATION lib
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)
