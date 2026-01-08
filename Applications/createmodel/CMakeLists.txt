get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB MIKKTSPACE_HEADERS "${mikktspace_SOURCE_DIR}/*.h")
file(GLOB MIKKTSPACE_SOURCES "${mikktspace_SOURCE_DIR}/*.c")

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES} ${MIKKTSPACE_HEADERS} ${MIKKTSPACE_SOURCES})

target_include_directories(${ProjectID} BEFORE PUBLIC "${mikktspace_SOURCE_DIR}")

target_link_libraries(${ProjectID} PRIVATE assimp argparse)
target_link_libraries(${ProjectID} PUBLIC Math Utility Shapes Resources)

install(TARGETS ${ProjectID} DESTINATION "")