get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB IMGUI_HEADERS "${imgui_SOURCE_DIR}/*.h")
file(GLOB IMGUI_SOURCES "${imgui_SOURCE_DIR}/*.cpp")
file(GLOB IMGUIZMO_HEADERS "${imguizmo_SOURCE_DIR}/*.h")
file(GLOB IMGUIZMO_SOURCES "${imguizmo_SOURCE_DIR}/*.cpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")

add_library(${ProjectID} STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS} ${IMGUIZMO_HEADERS} ${IMGUIZMO_SOURCES} ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ProjectID} BEFORE PUBLIC "${imgui_SOURCE_DIR}")
target_include_directories(${ProjectID} BEFORE PUBLIC "${imguizmo_SOURCE_DIR}")

target_link_libraries(${ProjectID} Math Utility Shapes)