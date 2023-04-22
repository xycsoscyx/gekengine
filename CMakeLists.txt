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

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
FetchContent_Declare(assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG master)
FetchContent_MakeAvailable(assimp)

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG master)
FetchContent_MakeAvailable(fmt)

add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_Populate(imgui)

FetchContent_Declare(imguizmo GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git GIT_TAG master)
FetchContent_Populate(imguizmo)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG master)
FetchContent_MakeAvailable(json)

FetchContent_Declare(mikktspace GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git GIT_TAG master)
FetchContent_Populate(mikktspace)

SET(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(newton GIT_REPOSITORY https://github.com/MADEAPPS/newton-dynamics.git GIT_TAG master)
FetchContent_Populate(newton)
add_subdirectory(${newton_SOURCE_DIR}/newton-4.00 ${newton_BINARY_DIR}/newton-4.00)
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dCharacter")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dVehicle")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dIkSolver")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dJoints")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton")

SET(TBB_TEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(oneTBB GIT_REPOSITORY https://github.com/oneapi-src/oneTBB GIT_TAG master)
FetchContent_MakeAvailable(oneTBB)

FetchContent_Declare(wink GIT_REPOSITORY https://github.com/miguelmartin75/Wink-Signals.git GIT_TAG main)
FetchContent_Populate(wink)
include_directories("${wink_SOURCE_DIR}")

if(GEK_BUILD_TESTS)
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG main)

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
    FetchContent_Declare(DirectXTex GIT_REPOSITORY https://github.com/microsoft/DirectXTex.git GIT_TAG main)
    FetchContent_MakeAvailable(DirectXTex)
endif()

add_definitions(-D_USE_MATH_DEFINES)
add_definitions(-DUNICODE -D_UNICODE)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)
endif()