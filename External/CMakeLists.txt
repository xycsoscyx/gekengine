project (External)

set_property(GLOBAL PROPERTY FOLDER "External")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/External")
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
target_include_directories(assimp PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/assimp/include")
message("${CMAKE_CURRENT_BINARY_DIR}/assimp/include")

if(WIN32)
    project(DirectXTex)
    add_definitions(-D_OPENMP)
    file(GLOB SOURCES "DirectXTex/DirectXTex/*.cpp" "DirectXTex/DirectXTex/*.h")
    add_library(DirectXTex STATIC ${SOURCES})
    target_include_directories(DirectXTex PUBLIC "${CMAKE_CURRENT_LIST_DIR}/DirectXTex/DirectXTex")
    #add_custom_command(TARGET DirectXTex PRE_BUILD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/DirectXTex/DirectXTex/Shaders" COMMAND "CompileShaders.cmd" )
endif()

project(imgui)
file(GLOB SOURCES "imgui-addons/*.cpp" "imgui-addons/*.h"
                  "imguizmo/*.cpp" "imguizmo/*.h"
                  "imgui_tabs/*.cpp" "imgui_tabs/*.h")
add_library(imgui STATIC ${SOURCES})
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_LIST_DIR}/imgui-addons"
                                        "${CMAKE_CURRENT_LIST_DIR}/imguizmo"
                                        "${CMAKE_CURRENT_LIST_DIR}/imgui_tabs")

project(nano-signal-slot)
add_library(nano-signal-slot INTERFACE)
target_include_directories(nano-signal-slot INTERFACE "${CMAKE_CURRENT_LIST_DIR}/nano-signal-slot")

set(NEWTON_3_14 True)

add_definitions(-D_NEWTON_STATIC_LIB -D_CUSTOM_JOINTS_STATIC_LIB -DPTW32_STATIC_LIB)

if(NEWTON_3_14)
	project(tinyxml)
	file(GLOB SOURCES "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml/*.h")
	add_library(tinyxml STATIC ${SOURCES})
	target_include_directories(tinyxml PUBLIC "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml")
	include_directories("${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/packages/thirdParty/tinyxml")

	SET(NEWTON_DEMOS_SANDBOX False CACHE BOOL "" FORCE)
	add_subdirectory("newton-dynamics")
endif(NEWTON_3_14)

if(NOT NEWTON_3_14)
	# Copied from master branch since 3.13 cmakes are broken
	add_definitions(-DHAVE_STRUCT_TIMESPEC)
	set(NEWTON_SOURCE "${CMAKE_CURRENT_LIST_DIR}/newton-dynamics/coreLibrary_300/source")

	file(GLOB dg_srcs "${NEWTON_SOURCE}/core/*.cpp")         # low level library
	file(GLOB physics_srcs "${NEWTON_SOURCE}/physics/*.cpp") # physics engine
	file(GLOB mesh_srcs "${NEWTON_SOURCE}/meshUtil/*.cpp")   # mesh geometry
	file(GLOB amp_srcs "${NEWTON_SOURCE}/ampPhysics/*.cpp")  # amp acceleration
	file(GLOB newton_srcs "${NEWTON_SOURCE}/newton/*.cpp")   # Newton engine

	if (WIN32)
	  # modified pthread library
	  file(GLOB_RECURSE pthread_srcs "${NEWTON_SOURCE}/pthreads.2/pthread.c")
	else (WIN32)
	  set(pthread_srcs "")
	endif (WIN32)

	add_library(NewtonObj OBJECT ${dg_srcs} ${physics_srcs} ${mesh_srcs} ${newton_srcs} ${amp_srcs} ${pthread_srcs})

	if (WIN32)
	  # add modified pthread library
	  target_include_directories(NewtonObj PRIVATE ${NEWTON_SOURCE}/pthreads.2)
	endif (WIN32)

	target_include_directories(NewtonObj PRIVATE ${NEWTON_SOURCE}/core)
	target_include_directories(NewtonObj PRIVATE ${NEWTON_SOURCE}/physics)
	target_include_directories(NewtonObj PRIVATE ${NEWTON_SOURCE}/meshUtil)
	target_include_directories(NewtonObj PRIVATE ${NEWTON_SOURCE}/ampPhysics)
	target_include_directories(NewtonObj INTERFACE ${NEWTON_SOURCE}/newton)

	# create static library
	add_library(NewtonStatic STATIC $<TARGET_OBJECTS:NewtonObj>)
	target_include_directories(NewtonStatic INTERFACE ${NEWTON_SOURCE}/newton)

	if (MSVC)
	  set_source_files_properties(${NEWTON_SOURCE}/core/dgTypes.cpp PROPERTIES COMPILE_FLAGS "/YcdgStdafx.h")
	  set_source_files_properties(${NEWTON_SOURCE}/newton/NewtonClass.cpp PROPERTIES COMPILE_FLAGS "/YcNewtonStdAfx.h")
	  set_source_files_properties(${NEWTON_SOURCE}/physics/dgWorld.cpp PROPERTIES COMPILE_FLAGS "/YcdgPhysicsStdafx.h")

	  set_target_properties(NewtonStatic PROPERTIES COMPILE_FLAGS "/YudgStdafx.h /YuNewtonStdAfx.h /YudgPhysicsStdafx.h")
	  set_target_properties (NewtonStatic PROPERTIES COMPILE_DEFINITIONS "_NEWTON_STATIC_LIB;_WIN_32_VER;PTW32_BUILD;PTW32_STATIC_LIB;_CRT_SECURE_NO_WARNINGS")
	endif(MSVC)
endif(NOT NEWTON_3_14)

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
set_property(TARGET uninstall PROPERTY FOLDER "External/Assimp")
if(WIN32)
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External/Assimp")
endif()

if(NEWTON_3_14)
	set_property(TARGET timeTracker PROPERTY FOLDER "External/Newton")
	set_property(TARGET Newton PROPERTY FOLDER "External/Newton")
endif(NEWTON_3_14)

set_property(TARGET NewtonObj PROPERTY FOLDER "External/Newton")
set_property(TARGET NewtonStatic PROPERTY FOLDER "External/Newton")

if(NEWTON_3_14)
	set_property(TARGET tinyxml PROPERTY FOLDER "External/dNewton")
	set_property(TARGET dAnimation PROPERTY FOLDER "External/dNewton")
	set_property(TARGET dContainers PROPERTY FOLDER "External/dNewton")
	set_property(TARGET dCustomJoints PROPERTY FOLDER "External/dNewton")
	set_property(TARGET dMath PROPERTY FOLDER "External/dNewton")
	set_property(TARGET dScene PROPERTY FOLDER "External/dNewton")
endif(NEWTON_3_14)

set_property(TARGET imgui PROPERTY FOLDER "External/ImGui")

if(WIN32)
    set_property(TARGET DirectXTex PROPERTY FOLDER "External/DirectXTex")
endif()
