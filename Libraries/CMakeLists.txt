project (Libraries)

add_subdirectory("Math")
add_subdirectory("Shapes")
add_subdirectory("Utility")
add_subdirectory("GUI")
add_subdirectory("Resources")

set_property(TARGET Math PROPERTY FOLDER "Libraries")
set_property(TARGET Shapes PROPERTY FOLDER "Libraries")
set_property(TARGET Utility PROPERTY FOLDER "Libraries")
set_property(TARGET GUI PROPERTY FOLDER "Libraries")