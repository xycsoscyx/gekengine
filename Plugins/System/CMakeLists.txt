get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

include(FetchContent)

set(SLANG_EXTERNAL_IMGUI_TARGET "imgui" CACHE STRING "Rename external imgui to prevent collision" FORCE)
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable slang examples" FORCE)
set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable slang gfx" FORCE)
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable slang tests" FORCE)
set(SLANG_ENABLE_SLANG_GLSLANG ON CACHE BOOL "Enable slang glslang wrapper target" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "Disable warnings-as-errors for SPIRV-Tools" FORCE)
set(SLANG_ENABLE_MIMALLOC OFF CACHE BOOL "Disable mimalloc in slang" FORCE)
set(SLANG_RHI_ENABLE_CUDA OFF CACHE BOOL "Disable CUDA backend" FORCE)
set(SLANG_RHI_ENABLE_OPTIX OFF CACHE BOOL "Disable OptiX backend" FORCE)
set(SLANG_SLANG_LLVM_FLAVOR DISABLE CACHE STRING "Do not build or fetch slang-llvm" FORCE)
set(SLANG_EMBED_CORE_MODULE ON CACHE BOOL "Enable embedded core module generation" FORCE)
set(SLANG_EMBED_CORE_MODULE_SOURCE ON CACHE BOOL "Enable embedded core module source generation" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build Slang third-party deps as static libs" FORCE)

FetchContent_Declare(slang
    GIT_REPOSITORY https://github.com/shader-slang/slang.git
    GIT_TAG v2026.2.2  
    GIT_SHALLOW TRUE
)    
FetchContent_MakeAvailable(slang)  

if(TARGET slang-neural-module)
    set_property(TARGET slang-neural-module PROPERTY EXCLUDE_FROM_ALL TRUE)
endif()

set(SLANG_INTERNAL_TARGETS 
    slang-rhi 
    slang-rhi-tests
    slang-compiler 
    slang-common-objects
    SPIRV-Tools 
    SPIRV-Tools-opt
)  

foreach(target_name ${SLANG_INTERNAL_TARGETS})
    if(TARGET ${target_name})  
        # Resolve aliases to modify the actual target properties
        get_target_property(real_target ${target_name} ALIASED_TARGET)
        if(NOT real_target)
            set(real_target ${target_name})
        endif()

        # Disable "Warnings as Errors" and silence specific C++20 deprecations
        target_compile_options(${real_target} PRIVATE "/WX-" "/wd4996" "/wd5232")
        
        # Silence MSVC's C++20 'is_pod' deprecation specifically
        target_compile_definitions(${real_target} PRIVATE _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING)
    endif()
endforeach()

file(GLOB API_HEADERS "${CMAKE_SOURCE_DIR}/Plugins/API/GEK/API/*.hpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
file(GLOB SOURCES "*.cpp")

list(APPEND SOURCES ${OS_SOURCES})

add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS} ${API_HEADERS})
set_target_properties(${ProjectID} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/core"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/core"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    OUTPUT_NAME $<IF:$<CONFIG:Debug>,${ProjectID}_debug,${ProjectID}>
    DEBUG_POSTFIX ""
)

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Get Slang include directories
get_target_property(slang_interface_includes slang INTERFACE_INCLUDE_DIRECTORIES)
if(slang_interface_includes)
    target_include_directories(${ProjectID} PRIVATE ${slang_interface_includes})
endif()
if(DEFINED slang_SOURCE_DIR)
    target_include_directories(${ProjectID} PRIVATE ${slang_SOURCE_DIR}/include)
endif()

target_link_libraries(${ProjectID} PUBLIC Math Utility Resources slang)
if(WIN32)
    target_link_libraries(${ProjectID} PRIVATE DirectXTex)
endif()

target_compile_definitions(${ProjectID} PRIVATE SLANG_STATIC)

install(TARGETS ${ProjectID}
    RUNTIME DESTINATION bin/plugins/core
    LIBRARY DESTINATION bin/plugins/core
    ARCHIVE DESTINATION lib
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)