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


# Add dependencies on utility tools so they are built before demo_engine
add_dependencies(${ProjectID} createmodel createhull createtree)
if(MSVC)
    add_dependencies(${ProjectID} compresstextures)
endif()

# Ensure all plugins are built/installed with demo_engine
add_dependencies(${ProjectID}
    renderd3d11
    rendervulkan
    # Add other plugin targets here as needed
)

# Ensure plugin installs run with main install
