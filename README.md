# Game Excelleration Kit

The Game Excelleration Kit, GEK, is a hobby project that I have been using to explore the latest C++ features, and the lastest in 3D graphics technology.  The engine itself is setup so that I can easily add new techniques without having to recompile the engine itself.  At it's core, the GEK engine uses plugins so that new features can easily be added without needing to recompile the base engine.  This also allows for switchable rendering backends so that multiple operating systems can be supported.  Currently, only Direct3D 11 is implemented, but additional support for Direct3D 12, OpenGL, and Vulkan, is in the works.

# Compiling

GEK can easily be compiled using CMake.  The GIT repository is setup to include all it's major dependencies as submodules.  The only additional dependency that is required is a DirectX/Windows SDK containing the Direct3D 11 API.  Checking out recursively will also check out the external submodules, and the CMake scripts are setup to generate the required projects and link against them as well.  So far only MSVC has been tested and verified, but additional support for alternate Windows compilers, as well as Linux and Mac, will be implemented and verified.

# Running

The GEK engine supports a context with configurable data paths.  By default, the context will look in it's own relative data directory, but most of the applications support an additional data path, which can be specified through an environment variable, to keep external data separate from the build, "gek_data_path".

```
data
\ filters (post processing filter definitions)
\ fonts (additional true type font location, used by ImGui)
\ materials (material definitions)
\ models (visual model data, used by model processor)
\ physics (physics model data, used by physics processor)
\ programs (shared location for rendering programs) 
\ scenes (scene definitions)
\ shaders (rendering shader definitions, defines the passes used to render models)
\ textures (textures)
\ visuals (visual model definitions, defines the model rendering pipelines)
```

The default data path includes everything needed to run demo_engine, which also supports the live editor.  Once run, pressing the ESC key will bring up the main menu, and Edit -> Show Editor can be selected.  From there, entities can be added to the scene, modified, and removed.  Disabling the editor will return to the standard engine view, but a FirstPersonCamera must be added with an empty target (the default framebuffer), for anything to be displayed.

Models can be converted to the GEK format using the createmodel application, which has a built in help information.  This utility uses Assimp to load from any supported format, and MikkTSpace to generate tangent information, then saves it in a custom binary format for quick loading in engine.  The converter also uses the standard GEK context, so the gek_data_path environment variable can be setup to include a custom search path.  The created files will be placed in a subdirectory of the main model file, even if that is in the external data location.

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

# License

GEK Engine is released under the MIT licence.
