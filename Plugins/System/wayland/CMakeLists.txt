cmake_minimum_required(VERSION 3.15)
project(systemwayland)

file(GLOB SOURCES "entrypoints.cpp" "WaylandDevice.cpp")

add_library(systemwayland SHARED ${SOURCES})
set_target_properties(systemwayland PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    OUTPUT_NAME $<IF:$<CONFIG:Debug>,systemwayland_debug,systemwayland>
    DEBUG_POSTFIX ""
)

target_link_libraries(systemwayland PUBLIC Math Utility Resources Common)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols)
pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

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

target_include_directories(systemwayland PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS} "${WL_GENERATED_DIR}")
target_link_directories(systemwayland PRIVATE ${WAYLAND_CLIENT_LIBRARY_DIRS})
target_link_libraries(systemwayland PRIVATE ${WAYLAND_CLIENT_LIBRARIES})
target_compile_options(systemwayland PRIVATE ${WAYLAND_CLIENT_CFLAGS_OTHER})

install(TARGETS systemwayland
    RUNTIME DESTINATION bin/plugins/system
    LIBRARY DESTINATION bin/plugins/system
    ARCHIVE DESTINATION lib
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)
