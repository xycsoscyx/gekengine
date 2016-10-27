cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})

target_link_libraries(${ProjectID} math utility)

set_target_properties(${ProjectID}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)