get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

include(FetchContent)

FetchContent_Declare(slang
    GIT_REPOSITORY https://github.com/shader-slang/slang.git
    GIT_TAG v2026.2.2  
    GIT_SHALLOW TRUE
)    
FetchContent_MakeAvailable(slang)  

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
file(GLOB SOURCES "*.cpp" "Window/${GEK_WINDOW_DEVICE}Device.cpp" "Render/${GEK_RENDER_DEVICE}Device.cpp" "Audio/${GEK_AUDIO_DEVICE}Device.cpp")

if(GEK_WINDOW_DEVICE STREQUAL "x11")
    find_package(X11 REQUIRED)
    list(APPEND API_HEADERS ${X11_INCLUDE_DIR})
endif()

if(GEK_RENDER_DEVICE STREQUAL "vulkan")
    # Vulkan configuration (find package early)
    set(VULKAN_SDK "C:/VulkanSDK/1.4.335.0")
    set(ENV{VULKAN_SDK} "${VULKAN_SDK}")
    find_package(Vulkan REQUIRED)
    list(APPEND API_HEADERS ${Vulkan_INCLUDE_DIRS})
endif()

list(APPEND SOURCES ${OS_SOURCES})

add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS} ${API_HEADERS})

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

if(GEK_WINDOW_DEVICE STREQUAL "x11")
    target_link_libraries(${ProjectID} PRIVATE X11::X11)
endif()

# Render device-specific linking
if(GEK_RENDER_DEVICE STREQUAL "d3d11")
    # D3D11 configuration: link required libraries
    target_link_libraries(${ProjectID} PRIVATE 
        d3d11 
        dxgi 
        d3dcompiler
    )
elseif(GEK_RENDER_DEVICE STREQUAL "vulkan")
    # Vulkan configuration: link Vulkan
    set(SLANG_EXTERNAL_IMGUI_TARGET "imgui" CACHE STRING "Rename external imgui to prevent collision" FORCE)
    set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "Disable slang examples" FORCE)
    set(SLANG_ENABLE_GFX OFF CACHE BOOL "Disable slang gfx" FORCE)
    set(SLANG_ENABLE_TESTS OFF CACHE BOOL "Disable slang tests" FORCE)
    set(SPIRV_WERROR OFF CACHE BOOL "Disable warnings-as-errors for SPIRV-Tools" FORCE)
    set(SLANG_LIBRARY_TYPE "STATIC" CACHE STRING "Build slang as a static library" FORCE)
    set(SLANG_ENABLE_MIMALLOC OFF CACHE BOOL "Disable mimalloc in slang" FORCE)
    set(SLANG_RHI_ENABLE_CUDA OFF CACHE BOOL "Disable CUDA backend" FORCE)
    set(SLANG_RHI_ENABLE_OPTIX OFF CACHE BOOL "Disable OptiX backend" FORCE)
    set(SLANG_SLANG_LLVM_FLAVOR DISABLE CACHE STRING "Do not build or fetch slang-llvm" FORCE)
    
    target_link_libraries(${ProjectID} PRIVATE 
        Vulkan::Vulkan 
    )
endif()

target_compile_definitions(${ProjectID} PRIVATE SLANG_STATIC)

install(TARGETS ${ProjectID} DESTINATION "bin")