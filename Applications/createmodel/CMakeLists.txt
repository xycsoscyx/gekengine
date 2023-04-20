get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB ARGPARSE_HEADERS "${CMAKE_SOURCE_DIR}/External/argparse/include/argparse/*.hpp")
file(GLOB MIKKTSPACE_HEADERS "${CMAKE_SOURCE_DIR}/External/MikkTSpace/*.h")
file(GLOB MIKKTSPACE_SOURCES "${CMAKE_SOURCE_DIR}/External/MikkTSpace/*.c")

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES} ${ARGPARSE_HEADERS} ${MIKKTSPACE_HEADERS} ${MIKKTSPACE_SOURCES})

target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/argparse/include")
target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/MikkTSpace")

target_link_libraries(${ProjectID} Math Utility Shapes Resources assimp)