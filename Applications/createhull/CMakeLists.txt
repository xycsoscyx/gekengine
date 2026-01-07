get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

add_definitions(-D_NEWTON_STATIC_LIB=1)

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})

target_link_libraries(${ProjectID} Math Utility Shapes Resources assimp argparse)

install(TARGETS ${ProjectID} DESTINATION "")