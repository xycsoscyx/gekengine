cmake_minimum_required(VERSION 3.6.0)

project (Applications)

add_subdirectory("Demo")
add_subdirectory("MaterialCreator")
add_subdirectory("ModelConverter")
add_subdirectory("SphericalHarmonicsGenerator")
add_subdirectory("TextureCompressor")

set_property(TARGET demo PROPERTY FOLDER "Applications")
set_property(TARGET materialcreator PROPERTY FOLDER "Applications")
set_property(TARGET modelconverter PROPERTY FOLDER "Applications")
set_property(TARGET sphericalharmonicsgenerator PROPERTY FOLDER "Applications")
set_property(TARGET texturecompressor PROPERTY FOLDER "Applications")