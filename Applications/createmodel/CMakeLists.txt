get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB MIKKTSPACE_HEADERS "${mikktspace_SOURCE_DIR}/*.h")
file(GLOB MIKKTSPACE_SOURCES "${mikktspace_SOURCE_DIR}/*.c")

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES} ${MIKKTSPACE_HEADERS} ${MIKKTSPACE_SOURCES})
set_target_properties(${ProjectID} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	OUTPUT_NAME $<IF:$<CONFIG:Debug>,${ProjectID}_debug,${ProjectID}>
	DEBUG_POSTFIX ""
)

target_include_directories(${ProjectID} BEFORE PUBLIC "${mikktspace_SOURCE_DIR}")

target_link_libraries(${ProjectID} PRIVATE assimp argparse)
target_link_libraries(${ProjectID} PUBLIC Math Utility Shapes Resources)

install(TARGETS ${ProjectID}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	CONFIGURATIONS Debug Release
	NAMELINK_SKIP
)