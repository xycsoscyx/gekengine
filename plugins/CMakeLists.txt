cmake_minimum_required(VERSION 3.6.0)

project (Plugins)

add_subdirectory("Components")
add_subdirectory("Engine")
add_subdirectory("Model")
add_subdirectory("Physics")
add_subdirectory("Shape")
add_subdirectory("System")

set_property(TARGET components PROPERTY FOLDER "Plugins")
set_property(TARGET engine PROPERTY FOLDER "Plugins")
set_property(TARGET model PROPERTY FOLDER "Plugins")
set_property(TARGET physics PROPERTY FOLDER "Plugins")
set_property(TARGET shape PROPERTY FOLDER "Plugins")
set_property(TARGET system PROPERTY FOLDER "Plugins")