get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})

target_link_libraries(${ProjectID} Math Utility System Resources argparse DirectXTex)

install(TARGETS ${ProjectID} DESTINATION "")