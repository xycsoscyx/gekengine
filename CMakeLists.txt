cmake_minimum_required(VERSION 3.5.2)

project (GEKEngine)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
endif()

add_subdirectory("external")
add_subdirectory("libraries")
add_subdirectory("plugins")
add_subdirectory("applications")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT demo)