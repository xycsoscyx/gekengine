cmake_minimum_required(VERSION 3.5.2)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option("GEK_BUILD_TESTS" "Create unit test projects" ON)

set("GEK_WINDOW_DEVICE" "win32" CACHE STRING "Compile targeting specific window device")
set_property(CACHE GEK_WINDOW_DEVICE PROPERTY STRINGS win32 x11)

set("GEK_RENDER_DEVICE" "d3d11" CACHE STRING "Compile targeting specific render device")
set_property(CACHE GEK_RENDER_DEVICE PROPERTY STRINGS d3d11 d3d12 vulkan)

set("GEK_AUDIO_DEVICE" "ds8" CACHE STRING "Compile targeting specific audio device")
set_property(CACHE GEK_AUDIO_DEVICE PROPERTY STRINGS ds8)

option("GEK_CREATE_FOLDERS" "Create project folders in generated solution" OFF)
if(MSVC)
    set(GEK_CREATE_FOLDERS ON CACHE BOOL "" FORCE)
endif()

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})
project(${ProjectID})

if(MSVC)
    if(GEK_CREATE_FOLDERS)
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    endif()

    add_definitions(/await:strict) # Enable coroutines
    add_definitions(/std:c++20)
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
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

FetchContent_Declare(argparse OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/p-ranav/argparse.git GIT_TAG master)
FetchContent_MakeAvailable(argparse)

if(MSVC)
    add_compile_options(/wd4702)
endif()

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
FetchContent_Declare(assimp OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG master)
FetchContent_MakeAvailable(assimp)

add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)
FetchContent_Declare(imgui OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_Populate(imgui)

FetchContent_Declare(imguizmo OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG master)
FetchContent_Populate(imguizmo)

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(json OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG master)
FetchContent_MakeAvailable(json)

FetchContent_Declare(mikktspace OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git GIT_TAG master)
FetchContent_Populate(mikktspace)

SET(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(newton OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/MADEAPPS/newton-dynamics.git GIT_TAG master SOURCE_SUBDIR newton-4.00)
FetchContent_MakeAvailable(newton)
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dCharacter")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dVehicle")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dIkSolver")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dJoints")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton")

SET(TBB_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(oneTBB OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/oneapi-src/oneTBB GIT_TAG master)
FetchContent_MakeAvailable(oneTBB)

FetchContent_Declare(wink OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/miguelmartin75/Wink-Signals.git GIT_TAG master)
FetchContent_Populate(wink)
include_directories("${wink_SOURCE_DIR}")

if(GEK_BUILD_TESTS)
    FetchContent_Declare(googletest OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG main)

    if(WIN32)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
endif()

if(WIN32)
    SET(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    SET(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    SET(BUILD_DX12 OFF CACHE BOOL "" FORCE)
    SET(BC_USE_OPENMP OFF CACHE BOOL "" FORCE)
    SET(DISABLE_MSVC_ITERATOR_DEBUGGING ON CACHE BOOL "" FORCE)
    FetchContent_Declare(DirectXTex OVERRIDE_FIND_PACKAGE GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git GIT_TAG main)
    FetchContent_MakeAvailable(DirectXTex)
endif()

add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-DNOMINMAX)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

if(GEK_CREATE_FOLDERS)
    set_property(TARGET assimp PROPERTY FOLDER "External")
    set_property(TARGET ndBrain PROPERTY FOLDER "External")
    set_property(TARGET ndFileFormat PROPERTY FOLDER "External")
    set_property(TARGET ndModel PROPERTY FOLDER "External")
    set_property(TARGET ndNewton PROPERTY FOLDER "External")
    set_property(TARGET ndSolverAvx2 PROPERTY FOLDER "External")
    set_property(TARGET ndTinyxml PROPERTY FOLDER "External")
    set_property(TARGET newton_tests PROPERTY FOLDER "External")
    set_property(TARGET openfbx PROPERTY FOLDER "External")
    set_property(TARGET tbb PROPERTY FOLDER "External")
    set_property(TARGET tbbmalloc PROPERTY FOLDER "External")
    set_property(TARGET tbbmalloc_proxy PROPERTY FOLDER "External")
    set_property(TARGET uninstall PROPERTY FOLDER "External")
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External")
    set_property(TARGET vhacd PROPERTY FOLDER "External")
    set_property(TARGET zlibstatic PROPERTY FOLDER "External")
    if(GEK_BUILD_TESTS)
        set_property(TARGET Math_test PROPERTY FOLDER "Tests")

        set_property(TARGET gmock PROPERTY FOLDER "External")
        set_property(TARGET gtest PROPERTY FOLDER "External")
        set_property(TARGET gmock_main PROPERTY FOLDER "External")
        set_property(TARGET gtest_main PROPERTY FOLDER "External")
    endif()
    if(WIN32)
        set_property(TARGET DirectXTex PROPERTY FOLDER "External")
    endif()
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)
endif()