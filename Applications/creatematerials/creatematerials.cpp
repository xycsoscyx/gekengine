#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/JSON.hpp"
#include <map>

using namespace Gek;

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    try
    {
        std::cout << "GEK Material Creator" << std::endl;

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, "Data"s));

		auto texturesPath(FileSystem::GetFileName(dataPath, "Textures"s).u8string());
		auto materialsPath(FileSystem::GetFileName(dataPath, "Materials"s));

		std::function<bool(FileSystem::Path const &)> findMaterials;
		findMaterials = [&](FileSystem::Path const &materialCollectionPath) -> bool
		{
			if (materialCollectionPath.isDirectory())
			{
				std::cout << "Collection Found: " << materialCollectionPath.c_str() << std::endl;
				FileSystem::Find(materialCollectionPath, [&](FileSystem::Path const &textureSetPath) -> bool
				{
					if (textureSetPath.isDirectory())
					{
						std::string materialName(textureSetPath.u8string());
                        materialName = materialName.substr(texturesPath.size() + 1);
						std::cout << "> Material Found: " << materialName.c_str() << std::endl;

                        JSON::Object renderState;
                        std::map<std::string, std::map<uint32_t, std::pair<FileSystem::Path, std::string>>> fileMap;
                        FileSystem::Find(textureSetPath, [&](FileSystem::Path const &filePath) -> bool
                        {
                            uint32_t extensionImportance = 0;
							std::string extension(String::GetLower(filePath.getExtension()));
                            if (extension == ".json"s)
                            {
                                renderState = JSON::Load(filePath);
                            }
                            else if (extension == ".dds"s)
                            {
                                extensionImportance = 0;
                            }
                            else if (extension == ".png"s)
                            {
                                extensionImportance = 1;
                            }
                            else if (extension == ".tga"s)
                            {
                                extensionImportance = 2;
                            }
                            else if (extension == ".jpg"s || extension == ".jpeg"s)
                            {
                                extensionImportance = 3;
                            }
                            else if (extension == ".bmp"s)
                            {
                                extensionImportance = 4;
                            }
                            else if (extension == ".tif"s || extension == ".tiff"s)
                            {
                                extensionImportance = 5;
                            }
                            else
                            {
                                return true;
                            }

                            std::string textureName(filePath.withoutExtension().u8string());
							textureName = String::GetLower(textureName.substr(texturesPath.size() + 1));
                            if (String::EndsWith(textureName, "basecolor"s) ||
                                String::EndsWith(textureName, "base_color"s) ||
                                String::EndsWith(textureName, "diffuse"s) ||
                                String::EndsWith(textureName, "diffuse_s"s) ||
                                String::EndsWith(textureName, "albedo"s) ||
                                String::EndsWith(textureName, "albedo_s"s) ||
                                String::EndsWith(textureName, "alb"s) ||
                                String::EndsWith(textureName, "_d"s) ||
                                String::EndsWith(textureName, "_c"s))
                            {
                                fileMap["albedo"s][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "normal"s) ||
                                String::EndsWith(textureName, "normalmap"s) ||
                                String::EndsWith(textureName, "normalmap_s"s) ||
                                String::EndsWith(textureName, "_n"s))
                            {
                                fileMap["normal"s][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "roughness"s) ||
                                String::EndsWith(textureName, "roughness_s"s) ||
                                String::EndsWith(textureName, "rough"s) ||
                                String::EndsWith(textureName, "_r"s))
                            {
                                fileMap["roughness"s][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "metalness"s) ||
                                String::EndsWith(textureName, "metallic"s) ||
                                String::EndsWith(textureName, "metal"s) ||
                                String::EndsWith(textureName, "_m"s))
                            {
                                fileMap["metallic"s][extensionImportance] = std::make_pair(filePath, textureName);
                            }

                            return true;
                        });

                        if (fileMap.count("albedo"s) > 0 && fileMap.count("normal"s) > 0)
                        {
                            JSON::Object dataNode;
                            for(auto &mapSearch : fileMap)
                            {
                                JSON::Object node;
                                node["file"s] = std::begin(mapSearch.second)->second.second;
                                if (mapSearch.first == "albedo"s)
                                {
                                    node["flags"s] = "sRGB"s;
                                }

                                dataNode.set(mapSearch.first, node);
                            }

                            auto materialPath(FileSystem::GetFileName(materialsPath, materialName).withExtension(".json"s));
                            FileSystem::MakeDirectoryChain(materialPath.getParentPath());

                            JSON::Object solidNode;
                            solidNode.set("data"s, dataNode);
                            if (!renderState.is_null())
                            {
                                solidNode.set("renderState"s, renderState);
                            }

                            JSON::Object passesNode;
                            passesNode.set("solid"s, solidNode);

                            JSON::Object shaderNode;
                            shaderNode.set("passes"s, passesNode);
                            shaderNode.set("name"s, "solid"s);

                            JSON::Object materialNode;
                            materialNode.set("shader"s, shaderNode);
                            JSON::Save(materialPath, materialNode);
                        }
					}

					return true;
				});
			}

			return true;
		};

		FileSystem::Find(texturesPath, findMaterials);
    }
    catch (const std::exception &exception)
    {
		std::cerr << "GEK Engine - Error" << std::endl;
		std::cerr << "Caught: " << exception.what() << std::endl;
		std::cerr << "Type: " << typeid(exception).name() << std::endl;
	}
    catch (...)
    {
        std::cerr << "GEK Engine - Error" << std::endl;
        std::cerr << "Caught: Non-standard exception" << std::endl;
    };

    return 0;
}