cmake_minimum_required(VERSION 3.10)

set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Installation prefix" FORCE)

# ============================================================================
# Configuration: Load options from CMakeOptions.txt if it exists
# This file allows you to easily configure build options without modifying 
# this CMakeLists.txt. To change options, edit CMakeOptions.txt and re-run 
# CMake configuration.
# 
# Options in CMakeOptions.txt override defaults set below.
# ============================================================================
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/CMakeOptions.txt")
    include("${CMAKE_CURRENT_LIST_DIR}/CMakeOptions.txt")
endif()

# Default options (used if CMakeOptions.txt doesn't set them)
option("GEK_BUILD_TESTS" "Create unit test projects" ON)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})
project(${ProjectID})

if(MSVC)
    add_definitions(/await:strict) # Enable coroutines
    add_definitions(/std:c++20)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
    add_compile_options(/FS)
    #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
endif()

if(NOT WIN32)
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif()

#add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC")
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -fPIC")
endif()

include(FetchContent)

# External dependency pinning policy:
# - Use immutable commit SHAs for new dependencies.
# - Update pins only via explicit dependency bump changes.
# - Avoid floating references such as master/main for reproducibility.
set(GEK_DEP_STB_GIT_TAG "31c1ad37456438565541f4919958214b6e762fb4")
set(GEK_DEP_BC7ENC_RDO_GIT_TAG "dbe416d28a5530b4e8cc45b14bf034dc6b96bbde")
set(GEK_DEP_BCDEC_GIT_TAG "93628fe5627102fe5187b7eeb99122dec6612c36")

FetchContent_Declare(argparse OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/p-ranav/argparse.git GIT_TAG v3.2 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(argparse)

if(MSVC)
    add_compile_options(/wd4702)
    add_compile_options(/wd4005)
    add_compile_options(/wd4365)
endif()

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
FetchContent_Declare(assimp OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v6.0.2 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(assimp)

add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)
FetchContent_Declare(imgui OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG 934c6a5f5ef2355d6df25395d555cb71f790c4e9 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(imguizmo OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG a15acd87a3f3241a29ea1363ceafc680dca3a96b OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(imguizmo)

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(mikktspace OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git GIT_TAG master OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(mikktspace)

SET(TBB_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(oneTBB OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/oneapi-src/oneTBB GIT_TAG v2022.3.0 OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(oneTBB)

FetchContent_Declare(wink OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/miguelmartin75/Wink-Signals.git GIT_TAG master OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(wink)
include_directories("${wink_SOURCE_DIR}")

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG ${GEK_DEP_STB_GIT_TAG}
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

FetchContent_Declare(bc7enc_rdo
    GIT_REPOSITORY https://github.com/richgel999/bc7enc_rdo.git
    GIT_TAG ${GEK_DEP_BC7ENC_RDO_GIT_TAG}
)
FetchContent_GetProperties(bc7enc_rdo)
if(NOT bc7enc_rdo_POPULATED)
    FetchContent_Populate(bc7enc_rdo)
endif()

FetchContent_Declare(bcdec
    GIT_REPOSITORY https://github.com/iOrange/bcdec.git
    GIT_TAG ${GEK_DEP_BCDEC_GIT_TAG}
)
FetchContent_GetProperties(bcdec)
if(NOT bcdec_POPULATED)
    FetchContent_Populate(bcdec)
endif()

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

add_library(bcdec INTERFACE)
target_include_directories(bcdec INTERFACE ${bcdec_SOURCE_DIR})

if(GEK_BUILD_TESTS)
    FetchContent_Declare(googletest OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG v1.17.0 OVERRIDE_FIND_PACKAGE)

    if(WIN32)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
endif()

# Add libktx for KTX2 texture support
set(KTX_FEATURE_TOOLS OFF CACHE BOOL "Disable KTX command-line tools in vendored builds" FORCE)
set(KTX_FEATURE_TESTS OFF CACHE BOOL "Disable KTX tests in vendored builds" FORCE)
FetchContent_Declare(
    ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG v4.0.0
)
FetchContent_MakeAvailable(ktx)
# Workaround: FetchContent causes CMAKE_SOURCE_DIR to resolve to the parent project root inside
# ktx's own CMakeLists.txt, making its PUBLIC_HEADER install paths point to gekengine/include/
# instead of ktx_SOURCE_DIR/include/. Override after population so the generated install script
# references the correct source location.
set_target_properties(ktx PROPERTIES
    PUBLIC_HEADER "${ktx_SOURCE_DIR}/include/ktx.h;${ktx_SOURCE_DIR}/include/ktxvulkan.h"
)

add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-DNOMINMAX)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)
endif()
