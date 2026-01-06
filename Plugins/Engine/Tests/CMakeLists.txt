get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB TESTS "*.cpp")
include(GoogleTest)
enable_testing()
add_executable(${ProjectID}_test ${TESTS})

target_link_libraries(${ProjectID}_test GTest::gtest GTest::gtest_main Engine Utility)

gtest_discover_tests(${ProjectID}_test)
