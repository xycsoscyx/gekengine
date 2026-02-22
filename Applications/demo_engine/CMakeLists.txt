get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_executable(${ProjectID} WIN32 ${SOURCES})

target_link_libraries(${ProjectID} PUBLIC Math Utility Engine Resources)

install(TARGETS ${ProjectID} DESTINATION "bin")

if(WIN32)
    add_custom_command(
        TARGET ${ProjectID} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${ProjectID}>"
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/../../cmake/CopyRuntimeDLLs.cmake"
            -D TARGET_DLLS="$<TARGET_RUNTIME_DLLS:${ProjectID}>"
            -D DEST_DIR="$<TARGET_FILE_DIR:${ProjectID}>"
        VERBATIM
    )
endif()
  