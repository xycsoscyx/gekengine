get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

add_definitions(-D_NEWTON_STATIC_LIB=1)

file(GLOB HEADERS "GEK/Newton/*.hpp")
file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${HEADERS} "${CMAKE_CURRENT_SOURCE_DIR}/../system" "${CMAKE_CURRENT_SOURCE_DIR}/../engine" "${CMAKE_CURRENT_SOURCE_DIR}/../components" "${CMAKE_CURRENT_SOURCE_DIR}/../model" "${CMAKE_CURRENT_SOURCE_DIR}/../shape")

target_link_libraries(${ProjectID} Math Utility Shapes nano-signal-slot imgui NewtonStatic)

set_target_properties(${ProjectID}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)