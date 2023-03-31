get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB IMGUI_HEADERS "${CMAKE_SOURCE_DIR}/External/imgui/*.h")
file(GLOB IMGUI_SOURCES "${CMAKE_SOURCE_DIR}/External/imgui/*.cpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")

add_library(${ProjectID} STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS} ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/imgui")

target_link_libraries(${ProjectID} Math Utility Shapes)