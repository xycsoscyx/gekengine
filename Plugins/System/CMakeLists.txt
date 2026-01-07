get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB API_HEADERS "${CMAKE_SOURCE_DIR}/Plugins/API/GEK/API/*.hpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
file(GLOB SOURCES "*.cpp" "Window/${GEK_WINDOW_DEVICE}Device.cpp" "Render/${GEK_RENDER_DEVICE}Device.cpp" "Audio/${GEK_AUDIO_DEVICE}Device.cpp")

if(GEK_RENDER_DEVICE STREQUAL "vulkan")
    set(VULKAN_SDK "C:/VulkanSDK/1.4.335.0")
    set(ENV{VULKAN_SDK} "${VULKAN_SDK}")
    find_package(Vulkan REQUIRED)
    list(APPEND API_HEADERS ${Vulkan_INCLUDE_DIRS})
endif()

if(GEK_WINDOW_DEVICE STREQUAL "x11")
    find_package(X11 REQUIRED)
    list(APPEND API_HEADERS ${X11_INCLUDE_DIR})
endif()

list(APPEND SOURCES ${OS_SOURCES})

add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS} ${API_HEADERS})

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} PRIVATE Math Utility Resources)
if(WIN32)
    target_link_libraries(${ProjectID} PRIVATE DirectXTex)
endif()

if(GEK_WINDOW_DEVICE STREQUAL "x11")
    target_link_libraries(${ProjectID} PRIVATE X11::X11)
endif()

if(GEK_RENDER_DEVICE STREQUAL "vulkan")
    target_link_libraries(${ProjectID} PRIVATE Vulkan::Vulkan)
endif()

install(TARGETS ${ProjectID} DESTINATION "")