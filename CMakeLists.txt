cmake_minimum_required(VERSION 3.5.2)

option("GEK_BUILD_TESTS" "Create unit test projects" ON)
option("GEK_CREATE_FOLDERS" "Create project folders in generated solution" ON)

project (GEKEngine)

if(MSVC)
    if(GEK_CREATE_FOLDERS)
        set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    endif()

	add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS -D_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(/wd4267)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
endif()

if (WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif ()

add_definitions(/wd4305) #
add_definitions(/wd4800) #
add_definitions(/wd4244) #
add_definitions(/wd4099) #
add_definitions(/wd4574) #
add_definitions(/wd4146) # unary minus operator applied to unsigned type, result still unsigned
add_definitions(/wd4100) # unreferenced formal parameter
add_definitions(/wd4101) # unreferenced local variable
add_definitions(/wd4005) # macro redefinition
add_definitions(/wd4820) # bytes padding added after data member
add_definitions(/wd4625) # copy constructor was implicitly defined as deleted
add_definitions(/wd4626) # assignment operator was implicitly defined as deleted
add_definitions(/wd5026) # move constructor was implicitly defined as deleted
add_definitions(/wd5027) # move assignment operator was implicitly defined as deleted
add_definitions(/wd4365) # conversion mismatch
add_definitions(/wd4312) # conversion to greater size
add_definitions(/wd4189) # local variable is initialized but not referenced
add_definitions(/wd4061) # enumerator in switch of enum is not explicitly handled by a case label
add_definitions(/wd4062) # enumerator in switch of enum is not handled
add_definitions(/wd4571) # Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
add_definitions(/wd5039) # pointer or reference to potentially throwing function passed to extern C function under -EHc. Undefined behavior may occur if this function throws an exception.
add_definitions(/wd4668) # not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
add_definitions(/wd4702) # unreachable code
add_definitions(/wd4710) # function not inlined
add_definitions(/wd4711) # function selected for automatic inline expansion
add_definitions(/wd4774) # format string expected in argument is not a string literal
add_definitions(/wd4777) # format string requires an argument of type, but variadic argument has type
add_definitions(/wd5045) # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
add_definitions(/wd5233) # explicit lambda capture 'this' is not used
add_definitions(/await:strict) # Enable coroutines
add_definitions(/std:c++latest) # Enable std::format
add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)
#add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4006")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")

include(FetchContent)

if(GEK_BUILD_TESTS)
    FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/b796f7d44681514f58a683a3a71ff17c94edb0c1.zip)

    if(WIN32)
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    FetchContent_MakeAvailable(googletest)
endif()

include_directories("${CMAKE_SOURCE_DIR}/External/Wink-Signals")

include_directories("${CMAKE_SOURCE_DIR}/External/json/single_include")

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

SET(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
SET(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
SET(BUILD_DX12 OFF CACHE BOOL "" FORCE)
SET(BC_USE_OPENMP OFF CACHE BOOL "" FORCE)
SET(DISABLE_MSVC_ITERATOR_DEBUGGING ON CACHE BOOL "" FORCE)
add_subdirectory("External/DirectXTex")

SET(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
add_subdirectory("External/newton-dynamics/newton-4.00")

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

set_target_properties(gtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(gtest_main PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(assimp PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set_target_properties(ndNewton PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

if(GEK_CREATE_FOLDERS)
    set_property(TARGET gmock PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gmock_main PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gtest PROPERTY FOLDER "External/Google Test")
    set_property(TARGET gtest_main PROPERTY FOLDER "External/Google Test")

    set_property(TARGET zlibstatic PROPERTY FOLDER "External/Assimp")
    set_property(TARGET assimp PROPERTY FOLDER "External/Assimp")
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

    if(WIN32)
        set_property(TARGET DirectXTex PROPERTY FOLDER "External/DirectXTex")
    endif()
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)
endif()