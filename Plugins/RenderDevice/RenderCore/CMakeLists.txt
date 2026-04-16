project(RenderCore)

file(GLOB API_HEADERS "${CMAKE_SOURCE_DIR}/Plugins/API/Engine/*.hpp")

file(GLOB RENDERCORE_HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
file(GLOB RENDERCORE_SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(RenderCore STATIC ${RENDERCORE_SOURCES} ${RENDERCORE_HEADERS})

set_target_properties(RenderCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    OUTPUT_NAME $<IF:$<CONFIG:Debug>,RenderCore_debug,RenderCore>
    DEBUG_POSTFIX ""
)

target_include_directories(RenderCore PUBLIC ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_SOURCE_DIR}/Plugins/API)

target_link_libraries(RenderCore PUBLIC Utility Math)

file(GLOB SOURCES "VideoDevice.cpp")

install(TARGETS RenderCore
    ARCHIVE DESTINATION lib
    CONFIGURATIONS Debug Release
    NAMELINK_SKIP
)
