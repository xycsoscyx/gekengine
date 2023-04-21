cmake_minimum_required(VERSION 3.5.2)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option("GEK_BUILD_TESTS" "Create unit test projects" ON)
option("GEK_CREATE_FOLDERS" "Create project folders in generated solution" ON)

project (GEKEngine)

if(MSVC)
    if(GEK_CREATE_FOLDERS)
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    endif()

    add_definitions(/await:strict) # Enable coroutines
    add_definitions(/std:c++20) # Enable std::format
	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4006")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")
endif()

if (WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
else()
    set(CMAKE_THREAD_LIBS_INIT "-lpthread")
    set(CMAKE_HAVE_THREADS_LIBRARY 1)
    set(CMAKE_USE_WIN32_THREADS_INIT 0)
    set(CMAKE_USE_PTHREADS_INIT 1)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
endif ()

#add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
add_definitions(-DNOMINMAX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(argparse GIT_REPOSITORY https://github.com/p-ranav/argparse.git GIT_TAG master)
FetchContent_MakeAvailable(argparse)

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG master)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG master)
FetchContent_MakeAvailable(json)

SET(TBB_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(oneTBB GIT_REPOSITORY https://github.com/oneapi-src/oneTBB GIT_TAG master)
FetchContent_MakeAvailable(oneTBB)

if(GEK_BUILD_TESTS)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/b796f7d44681514f58a683a3a71ff17c94edb0c1.zip)

    if(WIN32)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
endif()

include_directories("${CMAKE_SOURCE_DIR}/External/Wink-Signals")

include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dCore")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dCollision")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton/dModels")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton/dModels/dCharacter")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton/dModels/dVehicle")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton/dIkSolver")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton/dJoints")
include_directories("${CMAKE_SOURCE_DIR}/External/newton-dynamics/newton-4.00/sdk/dNewton")

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
add_subdirectory("External/Assimp")

add_definitions(-D_USE_MATH_DEFINES)

if(MSVC)
    SET(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    SET(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    SET(BUILD_DX12 OFF CACHE BOOL "" FORCE)
    SET(BC_USE_OPENMP OFF CACHE BOOL "" FORCE)
    SET(DISABLE_MSVC_ITERATOR_DEBUGGING ON CACHE BOOL "" FORCE)
    add_subdirectory("External/DirectXTex")
endif()

SET(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
add_subdirectory("External/newton-dynamics/newton-4.00")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

if(GEK_CREATE_FOLDERS)
    if(GEK_BUILD_TESTS)
        set_property(TARGET Math_test PROPERTY FOLDER "Tests")
    endif()

    set_property(TARGET tbb PROPERTY FOLDER "External/oneTBB")
    set_property(TARGET tbbmalloc PROPERTY FOLDER "External/oneTBB")
    #set_property(TARGET tbbmalloc_proxy PROPERTY FOLDER "External/oneTBB")

    set_property(TARGET gmock PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gmock_main PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gtest PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gtest_main PROPERTY FOLDER "External/Google Test")

    set_property(TARGET assimp PROPERTY FOLDER "External/Assimp")
    if(MSVC)
        set_property(TARGET zlibstatic PROPERTY FOLDER "External/Assimp")
    endif()
    set_property(TARGET uninstall PROPERTY FOLDER "External/Assimp")

    set_property(TARGET ndBrain PROPERTY FOLDER "External/Newton")
    set_property(TARGET ndFileFormat PROPERTY FOLDER "External/Newton")
    set_property(TARGET ndNewton PROPERTY FOLDER "External/Newton")
    set_property(TARGET ndSolverAvx2 PROPERTY FOLDER "External/Newton")
    set_property(TARGET ndTinyxml PROPERTY FOLDER "External/Newton")
    set_property(TARGET newton_tests PROPERTY FOLDER "External/Newton")
    set_property(TARGET openfbx PROPERTY FOLDER "External/Newton/Dependencies")
    set_property(TARGET vhacd PROPERTY FOLDER "External/Newton/Dependencies")
    if(WIN32)
        set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "External/Assimp")
    endif()

    if(MSVC)
        set_property(TARGET DirectXTex PROPERTY FOLDER "External/DirectXTex")
    endif()
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)
endif()