# Game Excelleration Kit

The Game Excelleration Kit, GEK, is a hobby project that I have been using to explore the latest C++ features, and the lastest in 3D graphics technology.  The engine itself is setup so that I can easily add new techniques without having to recompile the engine itself.  At it's core, the GEK engine uses plugins so that new features can easily be added without needing to recompile the base engine.  This also allows for switchable rendering backends so that multiple operating systems can be supported.  Currently, only Direct3D 11 is implemented, but additional support for Direct3D 12, OpenGL, and Vulkan, is in the works.

# Compiling

GEK can easily be compiled using CMake.  The GIT repository is setup to include all it's major dependencies as submodules.  The only additional dependency that is required is a DirectX/Windows SDK containing the Direct3D 11 API.  Checking out recursively will also check out the external submodules, and the CMake scripts are setup to generate the required projects and link against them as well.  So far only MSVC has been tested and verified, but additional support for alternate Windows compilers, as well as Linux and Mac, will be implemented and verified.

# Dependencies

GEK uses multiple external libraries for a number of functions.

## [Assimp](https://github.com/assimp/assimp)

The Open Asset Import Library is used to load 3D data from various formats.  GEK uses it's own internal format for physics and models, but includes a utility to prepare meshes for use in the engine, loading the data through Assimp, then converting it to the GEK format for quicker loading later.

## [DirectXTex](https://github.com/microsoft/DirectXTex)

DirectXTex is a shared source library for loading images in to Direct3D.  GEK uses this for the internal image loading, as well as a texture conversion utility to create optimal textures for the different rendering channels.

## [ImGui](https://github.com/ocornut/imgui)

Dear ImGui is an immediate mode UI that is used extensively by the internal features of GEK.

## [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)

ImGuizmo is an addon to ImGui to support movement gizmos in the world editor.

## [JSON](https://github.com/nlohmann/json)

The JSON library is a modern approach to loading JSON data in to C++.  GEK uses JSON for all of it's text information, from configs to materials to shaders, and uses the JSON library to load and parse the data.

## [Newton Dynamic](https://github.com/MADEAPPS/newton-dynamics)

Newton Dynamics is an integrated solution for real time simulation of physics environments.

## [Wink Signals](https://github.com/miguelmartin75/Wink-Signals)

Wink Signals is a standalone signals library, similar to boost::signals, this is used to store and triggere different events throughout the engine.

# License

GEK Engine is released under the MIT licence.
