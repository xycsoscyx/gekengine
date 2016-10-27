cmake_minimum_required(VERSION 3.6.0)

project (Libraries)

add_subdirectory("Math")
add_subdirectory("Shapes")
add_subdirectory("Utility")

set_property(TARGET math PROPERTY FOLDER "Libraries")
set_property(TARGET shapes PROPERTY FOLDER "Libraries")
set_property(TARGET utility PROPERTY FOLDER "Libraries")