get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})
set_target_properties(${ProjectID} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	OUTPUT_NAME $<IF:$<CONFIG:Debug>,${ProjectID}_debug,${ProjectID}>
	DEBUG_POSTFIX ""
)

target_link_libraries(${ProjectID} PRIVATE argparse DirectXTex)
target_link_libraries(${ProjectID} PUBLIC Math Utility Resources)

if (WIN32)
	target_link_libraries(${ProjectID} PRIVATE d3d11 dxgi)
endif()

install(TARGETS ${ProjectID}
	RUNTIME DESTINATION bin
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	COMPONENT Runtime
	CONFIGURATIONS Debug Release
	NAMELINK_SKIP
)