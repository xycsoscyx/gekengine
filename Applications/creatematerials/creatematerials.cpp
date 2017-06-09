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
        std::cout << "GEK Material Creator" << std::endl;

        auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
        auto dataPath(FileSystem::GetFileName(rootPath, "Data"));

		auto texturesPath(FileSystem::GetFileName(dataPath, "Textures").u8string());
		auto materialsPath(FileSystem::GetFileName(dataPath, "Materials"));

		std::function<bool(FileSystem::Path const &)> findMaterials;
		findMaterials = [&](FileSystem::Path const &materialCollectionPath) -> bool
		{
			if (materialCollectionPath.isDirectory())
			{
				std::cout << "Collection Found: " << materialCollectionPath.u8string() << std::endl;
				FileSystem::Find(materialCollectionPath, [&](FileSystem::Path const &textureSetPath) -> bool
				{
					if (textureSetPath.isDirectory())
					{
						std::string materialName(textureSetPath.u8string());
                        materialName = materialName.substr(texturesPath.size() + 1);
						std::cout << "> Material Found: " << materialName << std::endl;

                        jsoncons::json renderState;
                        std::map<std::string, std::map<uint32_t, std::pair<FileSystem::Path, std::string>>> fileMap;
                        FileSystem::Find(textureSetPath, [&](FileSystem::Path const &filePath) -> bool
                        {
                            std::cout << ">> File Found: " << filePath.u8string() << std::endl;

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
                                std::cout << "Found Albedo: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "normal") ||
                                String::EndsWith(textureName, "normalmap") ||
                                String::EndsWith(textureName, "normalmap_s") ||
                                String::EndsWith(textureName, "_n"))
                            {
                                fileMap["normal"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Normal: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "roughness") ||
                                String::EndsWith(textureName, "roughness_s") ||
                                String::EndsWith(textureName, "rough") ||
                                String::EndsWith(textureName, "_r"))
                            {
                                fileMap["roughness"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Roughness: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "specular") ||
                                String::EndsWith(textureName, "_s"))
                            {
                                fileMap["specular"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Specular: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "metalness") ||
                                String::EndsWith(textureName, "metallic") ||
                                String::EndsWith(textureName, "metal") ||
                                String::EndsWith(textureName, "_m"))
                            {
                                fileMap["metallic"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Metallic: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "clarity"))
                            {
                                fileMap["clarity"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Clarity: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "thickness"))
                            {
                                fileMap["thickness"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Thickness: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "height") ||
                                String::EndsWith(textureName, "displace"))
                            {
                                fileMap["height"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Height: " << filePath.u8string() << std::endl;
                            }
                            else if (String::EndsWith(textureName, "ambientocclusion") ||
                                String::EndsWith(textureName, "occlusion") ||
                                String::EndsWith(textureName, "ao"))
                            {
                                fileMap["occlusion"][extensionImportance] = std::make_pair(filePath, textureName);
                                std::cout << "Found Occlusion: " << filePath.u8string() << std::endl;
                            }
                            else
                            {
                                std::cout << "Unknown map type found: " << textureName << std::endl;
                            }

                            return true;
                        });

                        if (!fileMap.empty())
                        {
                            jsoncons::json dataNode;
                            for(auto &mapSearch : fileMap)
                            {
                                auto mapType(String::GetLower(mapSearch.first));
                                auto highestPriority(std::begin(mapSearch.second)->second);
                                auto filePath(highestPriority.first);
                                auto textureName(highestPriority.second);

                                if (FileSystem::Path(textureName).withoutExtension().getFileName() != mapType)
                                {
                                    auto sourceFilePath(filePath);
                                    filePath.replaceFileName(mapType + filePath.getExtension());
                                    textureName = FileSystem::Path(textureName).replaceFileName(mapType).u8string();
                                    std::cout << "Renaming " << sourceFilePath.u8string() << " to " << filePath.u8string() << ", named " << textureName << std::endl;
                                    std::experimental::filesystem::rename(sourceFilePath, filePath);
                                }

                                jsoncons::json node;
                                node["file"] = textureName;
                                if (mapType == "albedo")
                                {
                                    node["flags"] = "sRGB";
                                }

                                dataNode.set(mapType, node);
                            }

                            auto materialPath(FileSystem::GetFileName(materialsPath, materialName).withExtension(".json"));
                            FileSystem::MakeDirectoryChain(materialPath.getParentPath());

                            jsoncons::json solidNode;
                            solidNode.set("data", dataNode);
                            if (!renderState.is_null())
                            {
                                solidNode.set("renderState", renderState);
                            }

                            jsoncons::json passesNode;
                            passesNode.set("solid", solidNode);

                            jsoncons::json shaderNode;
                            shaderNode.set("passes", passesNode);
                            if (fileMap.count("clarity") > 0)
                            {
                                shaderNode.set("name", "glass");
                            }
                            else
                            {
                                shaderNode.set("name", "solid");
                            }

                            jsoncons::json materialNode;
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