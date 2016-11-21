cmake_minimum_required(VERSION 3.6.0)

project (Applications)

add_subdirectory("demo")
add_subdirectory("materialcreator")
add_subdirectory("modelconverter")
add_subdirectory("sphericalharmonicsgenerator")
add_subdirectory("texturecompressor")

set_property(TARGET demo PROPERTY FOLDER "Applications")
set_property(TARGET materialcreator PROPERTY FOLDER "Applications")
set_property(TARGET modelconverter PROPERTY FOLDER "Applications")
set_property(TARGET sphericalharmonicsgenerator PROPERTY FOLDER "Applications")
set_property(TARGET texturecompressor PROPERTY FOLDER "Applications")