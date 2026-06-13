get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

set(NEWTON_BUILD_SANDBOX_DEMOS OFF CACHE BOOL "" FORCE)
set(NEWTON_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Enforce consistent struct alignment for ndMatrix across all platforms.
# On Ubuntu, Newton may use AVX alignment (32-byte) by default, creating 96-byte ndMatrix,
# but our GEK Float4x4 is 64 bytes (16-byte SSE alignment). Force SSE alignment on both platforms.
add_compile_definitions(D_NEWTON_ENFORCE_CLASS_ALIGNMENT)
if(NOT MSVC)
    # Force SSE4.1 alignment (16-byte) on Unix/Linux to match Float4x4 layout
    add_compile_options(-msse4.1)
endif()
  
FetchContent_Declare(
	newton
	OVERRIDE_FIND_PACKAGE
	GIT_REPOSITORY https://github.com/JulioJerez/newton-dynamics
	GIT_TAG master
	SOURCE_SUBDIR newton-4.00
	OVERRIDE_FIND_PACKAGE)

FetchContent_MakeAvailable(newton)
  
# Apply alignment enforcement to Newton libraries as well
if(TARGET ndCore)
    target_compile_definitions(ndCore PUBLIC D_NEWTON_ENFORCE_CLASS_ALIGNMENT)
    if(NOT MSVC)
        target_compile_options(ndCore PRIVATE -msse4.1)
    endif()
endif()
if(TARGET ndNewton)
    target_compile_definitions(ndNewton PUBLIC D_NEWTON_ENFORCE_CLASS_ALIGNMENT)
    if(NOT MSVC)
        target_compile_options(ndNewton PRIVATE -msse4.1)
    endif()
endif()

include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCore/tinyxml")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dCollision/dMesh")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dCharacter")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dModels/dVehicle")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dIkSolver")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dJoints")
include_directories("${newton_SOURCE_DIR}/newton-4.00/sdk/dNewton/dMesh")
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