get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

add_library(${ProjectID} INTERFACE)
target_include_directories(${ProjectID} INTERFACE ${CMAKE_CURRENT_LIST_DIR})

file(TO_CMAKE_PATH "${CMAKE_CURRENT_LIST_DIR}/resource.rc" SOURCE)
target_sources(${ProjectID} INTERFACE ${SOURCE})