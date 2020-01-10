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

add_definitions(/wd4305 /wd4800 /wd4244 /wd4099 -DNOMINMAX)
add_definitions(-D_USE_MATH_DEFINES)

include_directories("${CMAKE_SOURCE_DIR}/External/Wink-Signals")

SET(ASSIMP_BUILD_IFC_IMPORTER False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_TOOLS False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_ASSIMP_VIEW False CACHE BOOL "" FORCE)
SET(ASSIMP_BUILD_TESTS False CACHE BOOL "" FORCE)
SET(ASSIMP_INSTALL_PDB False CACHE BOOL "" FORCE)
add_subdirectory("External/Assimp")

if(WIN32)
    add_subdirectory("External/DirectXTex")
endif()

add_subdirectory("External/jsoncons")

add_subdirectory("External/newton-dynamics")

message(WARNING "Silencing C++17 old allocator members deprecation warning")
add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)

message(WARNING "Silencing C++17 codecvt header deprecation warning")
add_definitions(-D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_definitions(-DUNICODE -D_UNICODE -D_HAS_CXX17)

add_subdirectory("Libraries")
add_subdirectory("Plugins")
add_subdirectory("Applications")

set_property(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY VS_STARTUP_PROJECT demo_render)