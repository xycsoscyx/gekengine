get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

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

file(GLOB IMGUI_HEADERS "${CMAKE_SOURCE_DIR}/External/imgui-addons/*.h")
file(GLOB IMGUI_SOURCES "${CMAKE_SOURCE_DIR}/External/imgui-addons/*.cpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")

add_library(${ProjectID} STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS} ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/imgui-addons")

target_link_libraries(${ProjectID} Math Utility Shapes)