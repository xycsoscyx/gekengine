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
        auto dataPath(FileSystem::GetFileName(rootPath, L"Data"));

        WString texturesPath(FileSystem::GetFileName(dataPath, L"Textures"));
        texturesPath.toLower();

        WString materialsPath(FileSystem::GetFileName(dataPath, L"Materials"));
        materialsPath.toLower();

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
						WString materialName(textureSetPath);
                        materialName = materialName.subString(texturesPath.size() + 1);
						std::cout << "> Material Found: " << materialName.c_str() << std::endl;

                        JSON::Object renderState;
                        std::map<WString, std::map<uint32_t, std::pair<FileSystem::Path, WString>>> fileMap;
                        FileSystem::Find(textureSetPath, [&](FileSystem::Path const &filePath) -> bool
                        {
                            uint32_t extensionImportance = 0;
                            WString extension(filePath.getExtension());
                            if (extension.compareNoCase(L".json") == 0)
                            {
                                try
                                {
                                    renderState = JSON::Load(filePath);
                                }
                                catch (...)
                                {
                                };
                            }
                            else if (extension.compareNoCase(L".dds") == 0)
                            {
                                extensionImportance = 0;
                            }
                            else if (extension.compareNoCase(L".png") == 0)
                            {
                                extensionImportance = 1;
                            }
                            else if (extension.compareNoCase(L".tga") == 0)
                            {
                                extensionImportance = 2;
                            }
                            else if (extension.compareNoCase(L".jpg") == 0 ||
                                extension.compareNoCase(L".jpeg") == 0)
                            {
                                extensionImportance = 3;
                            }
                            else if (extension.compareNoCase(L".bmp") == 0)
                            {
                                extensionImportance = 4;
                            }
                            else if (extension.compareNoCase(L".tif") == 0 ||
                                extension.compareNoCase(L".tiff") == 0)
                            {
                                extensionImportance = 5;
                            }
                            else
                            {
                                return true;
                            }

                            WString textureName(filePath.withoutExtension());
                            textureName = textureName.subString(texturesPath.size() + 1);
                            textureName.toLower();

                            if (textureName.endsWith(L"basecolor") ||
                                textureName.endsWith(L"base_color") ||
                                textureName.endsWith(L"diffuse") ||
                                textureName.endsWith(L"diffuse_s") ||
                                textureName.endsWith(L"albedo") ||
                                textureName.endsWith(L"albedo_s") ||
                                textureName.endsWith(L"alb") ||
                                textureName.endsWith(L"_d") ||
                                textureName.endsWith(L"_c"))
                            {
                                fileMap[L"albedo"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (textureName.endsWith(L"normal") ||
                                textureName.endsWith(L"normalmap") ||
                                textureName.endsWith(L"normalmap_s") ||
                                textureName.endsWith(L"_n"))
                            {
                                fileMap[L"normal"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (textureName.endsWith(L"roughness") ||
                                textureName.endsWith(L"roughness_s") ||
                                textureName.endsWith(L"rough") ||
                                textureName.endsWith(L"_r"))
                            {
                                fileMap[L"roughness"][extensionImportance] = std::make_pair(filePath, textureName);
                            }
                            else if (textureName.endsWith(L"metalness") ||
                                textureName.endsWith(L"metallic") ||
                                textureName.endsWith(L"metal") ||
                                textureName.endsWith(L"_m"))
                            {
                                fileMap[L"metallic"][extensionImportance] = std::make_pair(filePath, textureName);
                            }

                            return true;
                        });

                        if (fileMap.count(L"albedo") > 0 && fileMap.count(L"normal") > 0)
                        {
                            JSON::Object dataNode;
                            for(auto &mapSearch : fileMap)
                            {
                                JSON::Object node;
                                node[L"file"] = std::begin(mapSearch.second)->second.second;
                                if (mapSearch.first.compareNoCase(L"albedo") == 0)
                                {
                                    node[L"flags"] = L"sRGB";
                                }

                                dataNode.set(mapSearch.first, node);
                            }

                            auto materialPath(FileSystem::GetFileName(materialsPath, materialName).withExtension(L".json"));
                            FileSystem::MakeDirectoryChain(materialPath.getParentPath());

                            JSON::Object solidNode;
                            solidNode.set(L"data", dataNode);
                            if (!renderState.is_null())
                            {
                                solidNode.set(L"renderState", renderState);
                            }

                            JSON::Object passesNode;
                            passesNode.set(L"solid", solidNode);

                            JSON::Object shaderNode;
                            shaderNode.set(L"passes", passesNode);
                            shaderNode.set(L"name", L"solid");

                            JSON::Object materialNode;
                            materialNode.set(L"shader", shaderNode);
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