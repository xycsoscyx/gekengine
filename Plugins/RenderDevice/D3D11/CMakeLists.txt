include(FetchContent)

# Slang configuration (copied from System plugin)
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
		get_target_property(real_target ${target_name} ALIASED_TARGET)
		if(NOT real_target)
			set(real_target ${target_name})
		endif()
		target_compile_options(${real_target} PRIVATE "/WX-" "/wd4996" "/wd5232")
		target_compile_definitions(${real_target} PRIVATE _SILENCE_CXX20_IS_POD_DEPRECATION_WARNING)
	endif()
endforeach()
cmake_minimum_required(VERSION 3.15)
project(renderd3d11)

file(GLOB SOURCES "entrypoints.cpp" "D3D11Device.cpp")

add_library(renderd3d11 SHARED ${SOURCES})
set_target_properties(renderd3d11 PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/render"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/render"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	OUTPUT_NAME $<IF:$<CONFIG:Debug>,renderd3d11_debug,renderd3d11>
	DEBUG_POSTFIX ""
)

target_include_directories(renderd3d11 PUBLIC ${CMAKE_SOURCE_DIR}/Plugins/API)

# Slang include directories
get_target_property(slang_interface_includes slang INTERFACE_INCLUDE_DIRECTORIES)
if(slang_interface_includes)
	target_include_directories(renderd3d11 PRIVATE ${slang_interface_includes})
endif()
if(DEFINED slang_SOURCE_DIR)
	target_include_directories(renderd3d11 PRIVATE ${slang_SOURCE_DIR}/include)
endif()
target_link_libraries(renderd3d11 PUBLIC Math Utility Resources slang)
target_link_libraries(renderd3d11 PRIVATE d3d11 dxgi d3dcompiler DirectXTex)
set_target_properties(renderd3d11 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
install(TARGETS renderd3d11
    RUNTIME DESTINATION bin/plugins/render
    LIBRARY DESTINATION bin/plugins/render
    ARCHIVE DESTINATION lib
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)
