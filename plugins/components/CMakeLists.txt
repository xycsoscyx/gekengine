cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES})

target_include_directories(${ProjectID} PUBLIC ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/../system" "${CMAKE_CURRENT_LIST_DIR}/../engine")

target_link_libraries(${ProjectID} math utility shapes nano-signal-slot imgui)