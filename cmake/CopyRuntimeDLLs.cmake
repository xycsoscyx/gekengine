# Script to copy runtime DLLs to a destination directory
# This script is called after building test executables on Windows
#
# Arguments come as:
#   -D TARGET_DLLS="path1;path2;path3" -D DEST_DIR="..."
# With generator expressions like $<TARGET_RUNTIME_DLLS:target> expanded to semicolon-separated list

if(NOT DEFINED DEST_DIR OR DEST_DIR STREQUAL "")
    return()
endif()

if(NOT TARGET_DLLS OR TARGET_DLLS STREQUAL "")
    return()
endif()

# Create destination directory
file(MAKE_DIRECTORY "${DEST_DIR}")

# TARGET_DLLS is a semicolon-separated list of DLL file paths
# Split it and copy each one
foreach(DLL ${TARGET_DLLS})
    if(DLL AND EXISTS "${DLL}")
        get_filename_component(DLL_NAME "${DLL}" NAME)
        message(STATUS "CopyRuntimeDLLs: Copying ${DLL_NAME}")
        file(COPY "${DLL}" DESTINATION "${DEST_DIR}")
    endif()
endforeach()






