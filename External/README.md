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

## [MikkTSpace](https://github.com/mmikk/MikkTSpace)

MikkTSpace is the common format for storing tangent information, using a 3D vector for the tangent, and a single sign value, as opposed to calculating in the shader or requiring the additional bitangent/binormal.

## [oneTBB](https://github.com/oneapi-src/oneTBB)

Threading Building Blocks library, part of the oneAPI, which supports concurrency classes in a modern C++ style.

## [ArgParse](https://github.com/p-ranav/argparse)

C++ approach to parsing command line arguments, used by the applications to parse arguments, as well as show the command line options per application.
