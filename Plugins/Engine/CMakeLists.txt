get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[ch]pp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/../system" "${CMAKE_CURRENT_SOURCE_DIR}/../components" "${CMAKE_CURRENT_SOURCE_DIR}/../model")

target_link_libraries(${ProjectID} Math Shapes Utility GUI System signals)

set_target_properties(${ProjectID}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)