cmake_minimum_required(VERSION 3.6.0)

project (External)

set_property(GLOBAL PROPERTY FOLDER "External")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/external")
include(FindDirectX)
include(FindWindowsSDK)

SET(BUILD_SHARED_LIBS False CACHE BOOL "" FORCE)
SET(BUILD_STATIC_LIBS True CACHE BOOL "" FORCE)

SET(OPTION_BUILD_DOCS False CACHE BOOL "" FORCE)
SET(OPTION_BUILD_EXAMPLES False CACHE BOOL "" FORCE)
SET(OPTION_BUILD_GPU_TESTS False CACHE BOOL "" FORCE)
SET(OPTION_BUILD_TESTS False CACHE BOOL "" FORCE)
add_subdirectory("glbinding")

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
add_subdirectory("assimp")

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
SET(NEWTON_DEMOS_SANDBOX False CACHE BOOL "" FORCE)
add_subdirectory("newton-dynamics")

project(jsoncons)
add_library(jsoncons INTERFACE)
target_include_directories(jsoncons INTERFACE "${CMAKE_CURRENT_LIST_DIR}/jsoncons/src")

set_property(TARGET glmeta PROPERTY FOLDER "External/GLBinding")
set_property(TARGET component_install PROPERTY FOLDER "External/GLBinding")
set_property(TARGET pack PROPERTY FOLDER "External/GLBinding")
set_property(TARGET pack-glbinding PROPERTY FOLDER "External/GLBinding")
set_property(TARGET glbinding PROPERTY FOLDER "External/GLBinding")

set_property(TARGET zlibstatic PROPERTY FOLDER "External/Assimp")
set_property(TARGET assimp PROPERTY FOLDER "External/Assimp")
set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External/Assimp")
set_property(TARGET uninstall PROPERTY FOLDER "External/Assimp")

set_property(TARGET timeTracker PROPERTY FOLDER "External/Newton")
set_property(TARGET Newton PROPERTY FOLDER "External/Newton")
set_property(TARGET NewtonObj PROPERTY FOLDER "External/Newton")
set_property(TARGET NewtonStatic PROPERTY FOLDER "External/Newton")

set_property(TARGET tinyxml PROPERTY FOLDER "External/dNewton")
set_property(TARGET dAnimation PROPERTY FOLDER "External/dNewton")
set_property(TARGET dContainers PROPERTY FOLDER "External/dNewton")
set_property(TARGET dCustomJoints PROPERTY FOLDER "External/dNewton")
set_property(TARGET dMath PROPERTY FOLDER "External/dNewton")
set_property(TARGET dScene PROPERTY FOLDER "External/dNewton")

set_property(TARGET imgui PROPERTY FOLDER "External/ImGui")

if(WIN32)
    set_property(TARGET DirectXTex PROPERTY FOLDER "External/DirectXTex")
endif()
