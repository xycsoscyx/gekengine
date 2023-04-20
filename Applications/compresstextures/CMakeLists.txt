get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB ARGPARSE_HEADERS "${CMAKE_SOURCE_DIR}/External/argparse/include/argparse/*.hpp")

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES} ${ARGPARSE_HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/argparse/include")

target_link_libraries(${ProjectID} Math Utility System Resources DirectXTex)