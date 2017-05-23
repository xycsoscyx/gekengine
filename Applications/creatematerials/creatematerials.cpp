#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include <map>

using namespace Gek;

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    try
    {
        AtomicWriter() << "GEK Material Creator" << std::endl;

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, "Data"));

		auto texturesPath(FileSystem::GetFileName(dataPath, "Textures").u8string());
		auto materialsPath(FileSystem::GetFileName(dataPath, "Materials"));

		std::function<bool(FileSystem::Path const &)> findMaterials;
		findMaterials = [&](FileSystem::Path const &materialCollectionPath) -> bool
		{
			if (materialCollectionPath.isDirectory())
			{
				AtomicWriter() << "Collection Found: " << materialCollectionPath.u8string() << std::endl;
				FileSystem::Find(materialCollectionPath, [&](FileSystem::Path const &textureSetPath) -> bool
				{
					if (textureSetPath.isDirectory())
					{
						std::string materialName(textureSetPath.u8string());
                        materialName = materialName.substr(texturesPath.size() + 1);
						AtomicWriter() << "> Material Found: " << materialName.c_str() << std::endl;

                        JSON::Object renderState;
                        std::map<std::string, std::map<uint32_t, std::pair<FileSystem::Path, std::string>>> fileMap;
                        FileSystem::Find(textureSetPath, [&](FileSystem::Path const &filePath) -> bool
                        {
                            uint32_t extensionImportance = 0;
							std::string extension(String::GetLower(filePath.getExtension()));
                            if (extension == ".json")
                            {
                                renderState = JSON::Load(filePath);
                            }
                            else if (extension == ".dds")
                            {
                                extensionImportance = 0;
                            }
                            else if (extension == ".png")
                            {
                                extensionImportance = 1;
                            }
                            else if (extension == ".tga")
                            {
                                extensionImportance = 2;
                            }
                            else if (extension == ".jpg" || extension == ".jpeg")
                            {
                                extensionImportance = 3;
                            }
                            else if (extension == ".tif" || extension == ".tiff")
                            {
                                extensionImportance = 4;
                            }
                            else if (extension == ".bmp")
                            {
                                extensionImportance = 5;
                            }
                            else
                            {
                                return true;
                            }

                            std::string textureName(filePath.withoutExtension().u8string());
							textureName = String::GetLower(textureName.substr(texturesPath.size() + 1));
                            if (String::EndsWith(textureName, "basecolor") ||
                                String::EndsWith(textureName, "base_color") ||
                                String::EndsWith(textureName, "diffuse") ||
                                String::EndsWith(textureName, "diffuse_s") ||
                                String::EndsWith(textureName, "albedo") ||
                                String::EndsWith(textureName, "albedo_s") ||
                                String::EndsWith(textureName, "alb") ||
                                String::EndsWith(textureName, "_d") ||
                                String::EndsWith(textureName, "_c"))
                            {
                                fileMap["albedo"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "normal") ||
                                String::EndsWith(textureName, "normalmap") ||
                                String::EndsWith(textureName, "normalmap_s") ||
                                String::EndsWith(textureName, "_n"))
                            {
                                fileMap["normal"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "roughness") ||
                                String::EndsWith(textureName, "roughness_s") ||
                                String::EndsWith(textureName, "rough") ||
                                String::EndsWith(textureName, "_r"))
                            {
                                fileMap["roughness"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (String::EndsWith(textureName, "metalness") ||
                                String::EndsWith(textureName, "metallic") ||
                                String::EndsWith(textureName, "metal") ||
                                String::EndsWith(textureName, "_m"))
                            {
                                fileMap["metallic"][extensionImportance] = std::make_pair(filePath, textureName);
                            }

                            return true;
                        });

                        if (fileMap.count("albedo") > 0 && fileMap.count("normal") > 0)
                        {
                            JSON::Object dataNode;
                            for(auto &mapSearch : fileMap)
                            {
                                JSON::Object node;
                                node["file"] = std::begin(mapSearch.second)->second.second;
                                if (mapSearch.first == "albedo")
                                {
                                    node["flags"] = "sRGB";
                                }

                                dataNode.set(mapSearch.first, node);
                            }

                            auto materialPath(FileSystem::GetFileName(materialsPath, materialName).withExtension(".json"));
                            FileSystem::MakeDirectoryChain(materialPath.getParentPath());

                            JSON::Object solidNode;
                            solidNode.set("data", dataNode);
                            if (!renderState.is_null())
                            {
                                solidNode.set("renderState", renderState);
                            }

                            JSON::Object passesNode;
                            passesNode.set("solid", solidNode);

                            JSON::Object shaderNode;
                            shaderNode.set("passes", passesNode);
                            shaderNode.set("name", "solid");

                            JSON::Object materialNode;
                            materialNode.set("shader", shaderNode);
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
		AtomicWriter(std::cerr) << "GEK Engine - Error" << std::endl;
		AtomicWriter(std::cerr) << "Caught: " << exception.what() << std::endl;
		AtomicWriter(std::cerr) << "Type: " << typeid(exception).name() << std::endl;
	}
    catch (...)
    {
        AtomicWriter(std::cerr) << "GEK Engine - Error" << std::endl;
        AtomicWriter(std::cerr) << "Caught: Non-standard exception" << std::endl;
    };

    return 0;
}