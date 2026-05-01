get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_executable(${ProjectID} WIN32 ${SOURCES})
set_target_properties(${ProjectID} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME $<IF:$<CONFIG:Debug>,${ProjectID}_debug,${ProjectID}>
    DEBUG_POSTFIX ""
)

target_link_libraries(${ProjectID} PUBLIC Math Utility Engine Resources)

install(TARGETS ${ProjectID}
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT Runtime
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)


add_dependencies(${ProjectID} rendervulkan createmodel createhull createtree)
if(MSVC)
    add_dependencies(${ProjectID} compresstextures renderd3d11)
endif()

