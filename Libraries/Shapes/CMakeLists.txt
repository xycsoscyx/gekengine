get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")
add_library(${ProjectID} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} PUBLIC Math Utility)

if(GEK_BUILD_TESTS)
    file(GLOB TESTS "Tests/*.[hc]pp")
    include(GoogleTest)
    enable_testing()
    add_executable(${ProjectID}_test ${TESTS})
    target_link_libraries(${ProjectID}_test PRIVATE GTest::gtest GTest::gtest_main ${ProjectID})
    if(WIN32)
        add_custom_command(
            TARGET ${ProjectID}_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${ProjectID}_test>"
            COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/../../cmake/CopyRuntimeDLLs.cmake"
                -D TARGET_DLLS="$<TARGET_RUNTIME_DLLS:${ProjectID}_test>"
                -D DEST_DIR="$<TARGET_FILE_DIR:${ProjectID}_test>"
            VERBATIM
        )
    endif()
    gtest_discover_tests(${ProjectID}_test)
endif()