cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

add_definitions(-D_NEWTON_STATIC_LIB=1)

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})

target_include_directories(${ProjectID} PUBLIC ${CMAKE_CURRENT_LIST_DIR} "${CMAKE_CURRENT_LIST_DIR}/../system" "${CMAKE_CURRENT_LIST_DIR}/../engine" "${CMAKE_CURRENT_LIST_DIR}/../components" "${CMAKE_CURRENT_LIST_DIR}/../model" "${CMAKE_CURRENT_LIST_DIR}/../shape")

target_link_libraries(${ProjectID} math utility shapes nano-signal-slot imgui NewtonStatic)