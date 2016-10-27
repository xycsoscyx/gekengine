cmake_minimum_required(VERSION 3.6.0)

get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

add_definitions(-D_NEWTON_STATIC_LIB=1)

file(GLOB SOURCES "*.cpp" "*.rc")
add_executable(${ProjectID} ${SOURCES})

target_link_libraries(${ProjectID} math shapes utility NewtonStatic assimp)

target_include_directories(${ProjectID} PUBLIC "${CMAKE_SOURCE_DIR}/external/assimp/include")
