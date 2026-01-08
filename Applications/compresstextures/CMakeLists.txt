get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})

target_link_libraries(${ProjectID} PRIVATE argparse DirectXTex)
target_link_libraries(${ProjectID} PUBLIC Math Utility System Resources)

install(TARGETS ${ProjectID} DESTINATION "")