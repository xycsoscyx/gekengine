get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")
add_library(${ProjectID} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

if(GEK_BUILD_TESTS)
    file(GLOB TESTS "Tests/*.[hc]pp")
    include(GoogleTest)
    enable_testing()
    add_executable(${ProjectID}_test ${TESTS})
    target_link_libraries(${ProjectID}_test GTest::gtest GTest::gtest_main ${ProjectID})
    if(WIN32)
        add_custom_command(
            TARGET ${ProjectID}_test POST_BUILD
            # 1. Ensure the destination directory exists
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${ProjectID}_test>"
            
            # 2. Source files go first, destination directory goes last
            COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                    $<TARGET_RUNTIME_DLLS:${ProjectID}_test> 
                    "$<TARGET_FILE_DIR:${ProjectID}_test>"
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
    endif()
    gtest_discover_tests(${ProjectID}_test)
endif()