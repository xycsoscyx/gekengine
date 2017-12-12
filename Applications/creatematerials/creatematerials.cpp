#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/JSON.hpp"
#include <map>

using namespace Gek;

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{ std::cout } << "GEK Material Creator";

    auto rootPath(FileSystem::GetModuleFilePath().getParentPath().getParentPath());
    auto dataPath(FileSystem::CombinePaths(rootPath, "Data"));

	auto texturesPath(FileSystem::CombinePaths(dataPath, "textures").getString());
	auto materialsPath(FileSystem::CombinePaths(dataPath, "materials"));

	std::function<bool(FileSystem::Path const &)> findMaterials;
	findMaterials = [&](FileSystem::Path const &materialCollectionPath) -> bool
	{
		if (materialCollectionPath.isDirectory())
		{
			LockedWrite{ std::cout } << "Collection Found: " << materialCollectionPath.getString();
			materialCollectionPath.findFiles([&](FileSystem::Path const &textureSetPath) -> bool
			{
				if (textureSetPath.isDirectory())
				{
					std::string materialName(textureSetPath.getString());
                    materialName = materialName.substr(texturesPath.size() + 1);
					LockedWrite{ std::cout } << "> Material Found: " << materialName;

                    JSON::Object renderState;
                    std::map<std::string, std::map<uint32_t, std::pair<FileSystem::Path, std::string>>> fileMap;
                    textureSetPath.findFiles([&](FileSystem::Path const &filePath) -> bool
                    {
                        LockedWrite{ std::cout } << ">> File Found: " << filePath.getString();

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

                        std::string textureName(filePath.withoutExtension().getString());
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
                            LockedWrite{ std::cout } << "Found Albedo: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "normal") ||
                            String::EndsWith(textureName, "normalmap") ||
                            String::EndsWith(textureName, "normalmap_s") ||
                            String::EndsWith(textureName, "_n"))
                        {
                            fileMap["normal"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Normal: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "roughness") ||
                            String::EndsWith(textureName, "roughness_s") ||
                            String::EndsWith(textureName, "rough") ||
                            String::EndsWith(textureName, "_r"))
                        {
                            fileMap["roughness"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Roughness: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "specular") ||
                            String::EndsWith(textureName, "_s"))
                        {
                            fileMap["specular"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Specular: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "metalness") ||
                            String::EndsWith(textureName, "metallic") ||
                            String::EndsWith(textureName, "metal") ||
                            String::EndsWith(textureName, "_m"))
                        {
                            fileMap["metallic"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Metallic: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "clarity"))
                        {
                            fileMap["clarity"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Clarity: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "thickness"))
                        {
                            fileMap["thickness"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Thickness: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "height") ||
                            String::EndsWith(textureName, "displace"))
                        {
                            fileMap["height"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Height: " << filePath.getString();
                        }
                        else if (String::EndsWith(textureName, "ambientocclusion") ||
                            String::EndsWith(textureName, "occlusion") ||
                            String::EndsWith(textureName, "ao"))
                        {
                            fileMap["occlusion"][extensionImportance] = std::make_pair(filePath, textureName);
                            LockedWrite{ std::cout } << "Found Occlusion: " << filePath.getString();
                        }
                        else
                        {
                            LockedWrite{ std::cout } << "Unknown map type found: " << textureName;
                        }

                        return true;
                    });

                    if (!fileMap.empty())
                    {
                        JSON::Object dataNode;
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
                                textureName = FileSystem::Path(textureName).replaceFileName(mapType).getString();
                                LockedWrite{ std::cout } << "Renaming " << sourceFilePath.getString() << " to " << filePath.getString() << ", named " << textureName;
                                sourceFilePath.rename(filePath);
                            }

                            JSON::Object node;
                            node["file"] = textureName;
                            if (mapType == "albedo")
                            {
                                node["flags"] = "sRGB";
                            }

                            dataNode.set(mapType, node);
                        }

                        auto materialPath(FileSystem::CombinePaths(materialsPath, materialName + ".json"));
                        materialPath.getParentPath().createChain();

                        JSON::Object shaderNode;
                        shaderNode["data"] = dataNode;
                        if (!renderState.is_null())
                        {
                            shaderNode["renderState"] = renderState;
                        }

                        if (fileMap.count("clarity") > 0)
                        {
                            shaderNode["default"] = "glass";
                        }
                        else
                        {
                            shaderNode["default"] = "solid";
                        }

                        JSON::Object materialNode;
                        materialNode["shader"] = shaderNode;
                        JSON::Reference(materialNode).save(materialPath);
                    }
				}

				return true;
			});
		}

		return true;
	};

	FileSystem::Path(texturesPath).findFiles(findMaterials);
    return 0;
}