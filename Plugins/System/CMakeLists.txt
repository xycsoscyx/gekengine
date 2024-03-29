get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB API_HEADERS "${CMAKE_SOURCE_DIR}/Plugins/API/GEK/API/*.hpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
file(GLOB SOURCES "*.cpp" "Window/${GEK_WINDOW_DEVICE}Device.cpp" "Render/${GEK_RENDER_DEVICE}Device.cpp" "Audio/${GEK_AUDIO_DEVICE}Device.cpp")
if(WIN32)
    list(APPEND SOURCES ${OS_SOURCES})
else()
    find_package(X11 REQUIRED)
    find_package(Vulkan REQUIRED)
    list(APPEND API_HEADERS ${X11_INCLUDE_DIR} ${Vulkan_INCLUDE_DIRS})
    message(STATUS "Vuklan: ${Vulkan_INCLUDE_DIRS}, ${Vulkan_LIBRARIES}")
    message(STATUS "X11: ${X11_INCLUDE_DIR}, ${X11_LIBRARIES}")
    list(APPEND SOURCES ${OS_SOURCES})
endif()

add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS} ${API_HEADERS})

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} Math Utility Resources)
if(WIN32)
    target_link_libraries(${ProjectID} DirectXTex)
else()
    target_link_libraries(${ProjectID} ${X11_LIBRARIES} ${Vulkan_LIBRARIES})
endif()

install(TARGETS ${ProjectID} DESTINATION "bin")