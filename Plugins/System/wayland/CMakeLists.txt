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

target_include_directories(systemwayland PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
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
