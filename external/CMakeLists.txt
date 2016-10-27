cmake_minimum_required(VERSION 3.6.0)

project (External)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/external")
include(FindDirectX)
include(FindWindowsSDK)

SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
SET(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)

add_subdirectory("TinyXML2")

SET(ASSIMP_BUILD_IFC_IMPORTER OFF CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW OFF CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("Assimp")

if(WIN32)
    project(DirectXTex)
    file(GLOB SOURCES "DirectXTex/DirectXTex/*.cpp" "DirectXTex/DirectXTex/*.h")
    add_library(DirectXTex STATIC ${SOURCES})
    target_include_directories(DirectXTex PUBLIC "${CMAKE_CURRENT_LIST_DIR}/DirectXTex/DirectXTex")
    add_custom_command(TARGET DirectXTex PRE_BUILD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/DirectXTex/Shaders" COMMAND "CompileShaders.cmd" )
endif()

project(imgui)
file(GLOB SOURCES "imgui/*.cpp" "imgui/*.h" "imguizmo/*.cpp" "imguizmo/*.h")
add_library(imgui STATIC ${SOURCES})
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_LIST_DIR}/imgui" "${CMAKE_CURRENT_LIST_DIR}/imguizmo")

project(nano-signal-slot)
add_library(nano-signal-slot INTERFACE)
target_include_directories(nano-signal-slot INTERFACE "${CMAKE_CURRENT_LIST_DIR}/nano-signal-slot")

project(tinyxml)
file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml/*.h")
add_library(tinyxml STATIC ${SOURCES})
target_include_directories(tinyxml PUBLIC "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml")
include_directories("${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml")

add_definitions(-D_NEWTON_STATIC_LIB -D_CUSTOM_JOINTS_STATIC_LIB -DPTW32_STATIC_LIB)
SET(NEWTON_DEMOS_SANDBOX OFF CACHE BOOL "" FORCE)
add_subdirectory("Newton-Dynamics")
