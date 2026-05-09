get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

# 1. Set variables that Newton's CMakeLists.txt expects
set(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
set(NEWTON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# 2. Add the alignment definition to the current directory scope.
# When MakeAvailable calls add_subdirectory, Newton will inherit this.
add_compile_definitions(D_NEWTON_ENFORCE_CLASS_ALIGNMENT)

# 3. Declare the dependency
FetchContent_Declare(
    newton
    GIT_REPOSITORY "https://github.com"
    GIT_TAG master
    SOURCE_SUBDIR newton-4.00
)

# 4. Use the modern 'intended' command
# This handles the download, configuration, and build setup in one step.
FetchContent_MakeAvailable(newton)

include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore/tinyxml")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision/dMesh")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dCharacter")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dVehicle")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dIkSolver")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dJoints")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton")

file(GLOB HEADERS "GEK/Newton/*.hpp")
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

target_link_libraries(${ProjectID} PRIVATE ndNewton)
target_link_libraries(${ProjectID} PUBLIC Math Shapes Utility GUI Common Components Model)

install(TARGETS ${ProjectID}
	RUNTIME DESTINATION bin/plugins/core
	LIBRARY DESTINATION bin/plugins/core
	ARCHIVE DESTINATION lib
	CONFIGURATIONS Debug Release
	NAMELINK_SKIP
)