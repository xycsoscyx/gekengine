cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES})

target_include_directories(${ProjectID} PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${ProjectID} math utility DirectXTex)