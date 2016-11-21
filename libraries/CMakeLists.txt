cmake_minimum_required(VERSION 3.6.0)

project (Libraries)

add_subdirectory("math")
add_subdirectory("shapes")
add_subdirectory("utility")

set_property(TARGET math PROPERTY FOLDER "Libraries")
set_property(TARGET shapes PROPERTY FOLDER "Libraries")
set_property(TARGET utility PROPERTY FOLDER "Libraries")