cmake_minimum_required(VERSION 3.5.2)

project (GEKEngine)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(/wd4267)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
endif()

add_definitions(/wd4305 /wd4800 /wd4244 /wd4099 -DNOMINMAX)

add_subdirectory("External")

add_definitions(-DIMGUI_INCLUDE_IMGUI_USER_H -DIMGUI_INCLUDE_IMGUI_USER_INL)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-DUNICODE -D_UNICODE -D_HAS_CXX17)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT demo_render)