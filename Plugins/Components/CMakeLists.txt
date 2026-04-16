get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})
set_target_properties(${ProjectID} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/core"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/core"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	OUTPUT_NAME $<IF:$<CONFIG:Debug>,${ProjectID}_debug,${ProjectID}>
	DEBUG_POSTFIX ""
)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} PUBLIC Math Shapes Utility GUI Common)

install(TARGETS ${ProjectID}
	RUNTIME DESTINATION bin/plugins/core
	LIBRARY DESTINATION bin/plugins/core
	ARCHIVE DESTINATION lib
	CONFIGURATIONS Debug Release
	NAMELINK_SKIP
)
