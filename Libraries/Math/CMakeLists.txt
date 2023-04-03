get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.[hc]pp")
add_library(${ProjectID} STATIC ${SOURCES} ${HEADERS})
target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})

file(GLOB TESTS "Tests/*.[hc]pp")
enable_testing()
add_executable(${ProjectID}_test ${TESTS})
target_link_libraries(${ProjectID}_test GTest::gtest_main ${ProjectID})
include(GoogleTest)
gtest_discover_tests(${ProjectID}_test)

