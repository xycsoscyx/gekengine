get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")

file(GLOB SOURCES "*.cpp" "Devices/${GEK_RENDER_DEVICE}Device.cpp")

add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} PUBLIC Math Utility Resources DirectXTex)

# Link lodepng if available (provides lodepng.h). This fixes builds when the OpenGL device
# needs to include "lodepng.h" from Newton's thirdParty.
if(TARGET lodepng)
    target_link_libraries(${ProjectID} PRIVATE lodepng)
else()
    # Fallback: if the newton thirdParty png directory exists in the build tree, add it to includes.
    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/png/lodepng.h")
        target_include_directories(${ProjectID} PRIVATE "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/png")
    elseif(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/png/lodepng.h")
        target_include_directories(${ProjectID} PRIVATE "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/png")
    endif()
endif()

# Ensure the lodepng header is visible for compilation when present in Newton's thirdParty.
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/png/lodepng.h")
    target_include_directories(${ProjectID} PRIVATE "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/png")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/png/lodepng.h")
    target_include_directories(${ProjectID} PRIVATE "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/png")
endif()

# Link glatter/glfw if available to get GL headers & loader targets
if(TARGET glatter)
    target_link_libraries(${ProjectID} PRIVATE glatter)
endif()
if(TARGET glfw)
    target_link_libraries(${ProjectID} PRIVATE glfw)
endif()

# Ensure glatter headers are available when building the Render plugin
if(EXISTS "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/glatter/include")
    target_include_directories(${ProjectID} PRIVATE "${CMAKE_BINARY_DIR}/_deps/newton-src/newton-4.00/thirdParty/glatter/include")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/glatter/include")
    target_include_directories(${ProjectID} PRIVATE "${CMAKE_SOURCE_DIR}/build/_deps/newton-src/newton-4.00/thirdParty/glatter/include")
endif()

# Also add the System GEK include path so the Render plugin can reference system-level render interfaces
if(EXISTS "${CMAKE_SOURCE_DIR}/Plugins/System/GEK")
    target_include_directories(${ProjectID} PRIVATE "${CMAKE_SOURCE_DIR}/Plugins/System/GEK")
endif()

install(TARGETS ${ProjectID} DESTINATION "bin")