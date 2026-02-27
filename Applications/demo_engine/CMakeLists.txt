get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_executable(${ProjectID} WIN32 ${SOURCES})

target_link_libraries(${ProjectID} PUBLIC Math Utility Engine Resources)

install(TARGETS ${ProjectID} DESTINATION "bin" COMPONENT Runtime)

if(WIN32)
    install(FILES $<TARGET_RUNTIME_DLLS:${ProjectID}> DESTINATION "bin" COMPONENT Runtime)
endif()
  