get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_executable(${ProjectID} WIN32 ${SOURCES})

target_link_libraries(${ProjectID} Math Utility Engine Resources)

install(TARGETS ${ProjectID} DESTINATION "bin")

if(WIN32)
    add_custom_command(
        TARGET ${ProjectID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${ProjectID}> $<TARGET_FILE_DIR:${ProjectID}>
        COMMAND_EXPAND_LISTS
    )
endif()
