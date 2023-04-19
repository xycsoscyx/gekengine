get_filename_component(ProjectID ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectID ${ProjectID})

project(${ProjectID})

file(GLOB API_HEADERS "${CMAKE_SOURCE_DIR}/Plugins/API/GEK/API/*.hpp")

file(GLOB HEADERS "GEK/${ProjectID}/*.hpp")
file(GLOB SOURCES "*.cpp")
add_library(${ProjectID} SHARED ${SOURCES} ${HEADERS} ${API_HEADERS})

set_property(TARGET ${ProjectID} PROPERTY WINDOWS_EXPORT_ALL_SYMBOLS True)

target_include_directories(${ProjectID} BEFORE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${ProjectID} BEFORE PUBLIC "${CMAKE_SOURCE_DIR}/External/DirectXTex")

target_link_libraries(${ProjectID} DirectXTex Math Utility Resources)

set_target_properties(${ProjectID}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)