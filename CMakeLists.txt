cmake_minimum_required(VERSION 3.5.2)

project (GEKEngine)

if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
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
add_definitions(-D_USE_MATH_DEFINES -DNOMINMAX)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4006")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4221")

set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4006")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")

include_directories("${CMAKE_SOURCE_DIR}/External/Wink-Signals")

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
add_subdirectory("External/Assimp")

add_subdirectory("External/DirectXTex")

SET(BUILD_TESTS False CACHE BOOL "" FORCE)
add_subdirectory("External/jsoncons")

SET(NEWTON_BUILD_SANDBOX_DEMOS False CACHE BOOL "" FORCE)
SET(NEWTON_BUILD_PROFILER False CACHE BOOL "" FORCE)
#SET(NEWTON_STATIC_RUNTIME_LIBRARIES False CACHE BOOL "" FORCE)
add_subdirectory("External/newton-dynamics")

message(WARNING "Silencing C++17 old allocator members deprecation warning")
add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)

message(WARNING "Silencing C++17 codecvt header deprecation warning")
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-DUNICODE -D_UNICODE -D_HAS_CXX17)

add_definitions(-DNO_IMGUITOOLBAR)
add_definitions(-DNO_IMGUIPANELMANAGER)
add_definitions(-DNO_IMGUISTRING)
add_definitions(-DNO_IMGUIHELPER)
add_definitions(-DNO_IMGUIFILESYSTEM)
add_definitions(-DNO_IMGUICODEEDITOR)
add_definitions(-DNO_IMGUILISTVIEW)
add_definitions(-DNO_IMGUINODEGRAPHEDITOR)
add_definitions(-DIMGUI_INCLUDE_IMGUI_USER_H -DIMGUI_INCLUDE_IMGUI_USER_INL)
add_definitions(-DIMGUI_DEFINE_MATH_OPERATORS -DIMGUI_DEFINE_PLACEMENT_NEW)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_engine)