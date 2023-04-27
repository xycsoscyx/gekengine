get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS})

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/DirectXTex/DirectXTex")

target_link_libraries(${ProjectID} DirectXTex Math Utility Resources)

install(TARGETS ${ProjectID} DESTINATION "bin")