cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

project(${ProjectId})

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectId} ${SOURCES})