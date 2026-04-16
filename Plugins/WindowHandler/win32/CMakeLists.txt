cmake_minimum_required(VERSION 3.15)
project(systemwin32)

file(GLOB SOURCES "entrypoints.cpp" "Win32Device.cpp")

add_library(systemwin32 SHARED ${SOURCES})
set_target_properties(systemwin32 PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins/system"
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	OUTPUT_NAME $<IF:$<CONFIG:Debug>,systemwin32_debug,systemwin32>
	DEBUG_POSTFIX ""
)

target_include_directories(systemwin32 PUBLIC ${CMAKE_SOURCE_DIR}/Plugins/API)

target_link_libraries(systemwin32 PUBLIC Math Utility Resources)
target_link_libraries(systemwin32 PRIVATE user32 gdi32)
set_target_properties(systemwin32 PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
install(TARGETS systemwin32
	RUNTIME DESTINATION bin/plugins/system
	LIBRARY DESTINATION bin/plugins/system
	ARCHIVE DESTINATION lib
	CONFIGURATIONS Debug Release
	NAMELINK_SKIP
)
