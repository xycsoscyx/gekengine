project (Libraries)

add_subdirectory("Math")
add_subdirectory("Shapes")
add_subdirectory("Utility")

set_property(TARGET Math PROPERTY FOLDER "Libraries")
set_property(TARGET Shapes PROPERTY FOLDER "Libraries")
set_property(TARGET Utility PROPERTY FOLDER "Libraries")