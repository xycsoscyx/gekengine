#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include <experimental\filesystem>
#include <Windows.h>
#include <map>

using namespace Gek;

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    try
    {
        printf("GEK Material Creator\r\n");

		String rootPath;
		String currentModuleName((MAX_PATH + 1), L' ');
		GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
		currentModuleName.trimRight();

		String fullModuleName((MAX_PATH + 1), L' ');
		GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
		fullModuleName.trimRight();

		std::experimental::filesystem::path fullModulePath(fullModuleName);
		fullModulePath.remove_filename();
		fullModulePath.remove_filename();
		rootPath = fullModulePath.append(L"Data").wstring();

		String texturesPath(FileSystem::GetFileName(rootPath, L"Textures").getLower());
		String materialsPath(FileSystem::GetFileName(rootPath, L"Materials").getLower());

		std::function<bool(const wchar_t *)> findMaterials;
		findMaterials = [&](const wchar_t *setDirectoryName) -> bool
		{
			if (FileSystem::IsDirectory(setDirectoryName))
			{
				printf("Set Found: %S\r\n", setDirectoryName);
				FileSystem::Find(setDirectoryName, [&](const wchar_t *textureSetPath) -> bool
				{
					if (FileSystem::IsDirectory(textureSetPath))
					{
						String materialName(textureSetPath);
						materialName.replace((texturesPath + L"\\"), L"");
						printf("> Material Found: %S\r\n", materialName.c_str());

                        JSON::Object renderState;
                        std::map<String, std::map<uint32_t, std::pair<String, String>>> fileMap;
                        FileSystem::Find(textureSetPath, [&](const wchar_t *fileName) -> bool
                        {
                            uint32_t extensionImportance = 0;
                            String extension(FileSystem::GetExtension(fileName));
                            if (extension.compareNoCase(L".json") == 0)
                            {
                                try
                                {
                                    renderState = JSON::Load(fileName);
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
                            else
                            {
                                return true;
                            }

                            String textureName(FileSystem::ReplaceExtension(fileName).getLower());
                            textureName.replace((texturesPath + L"\\"), L"");
                            textureName.toLower();

                            if (textureName.endsWith(L"basecolor") ||
                                textureName.endsWith(L"base_color") ||
                                textureName.endsWith(L"diffuse") ||
                                textureName.endsWith(L"albedo") ||
                                textureName.endsWith(L"alb") ||
                                textureName.endsWith(L"_d"))
                            {
                                fileMap[L"albedo"][extensionImportance] = std::make_pair(fileName, textureName);
                            }
                            else if (textureName.endsWith(L"normal") ||
                                textureName.endsWith(L"_n"))
                            {
                                fileMap[L"normal"][extensionImportance] = std::make_pair(fileName, textureName);
                            }
                            else if (textureName.endsWith(L"roughness") ||
                                textureName.endsWith(L"rough") ||
                                textureName.endsWith(L"_r"))
                            {
                                fileMap[L"roughness"][extensionImportance] = std::make_pair(fileName, textureName);
                            }
                            else if (textureName.endsWith(L"metalness") ||
                                textureName.endsWith(L"metallic") ||
                                textureName.endsWith(L"metal") ||
                                textureName.endsWith(L"_m"))
                            {
                                fileMap[L"metallic"][extensionImportance] = std::make_pair(fileName, textureName);
                            }

                            return true;
                        });

                        if (fileMap.count(L"albedo") > 0 && fileMap.count(L"normal") > 0)
                        {
                            JSON::Object dataNode;
                            for(auto &mapSearch : fileMap)
                            {
                                auto fileName = mapSearch.second.begin()->second.first;
                                auto textureName = mapSearch.second.begin()->second.second;

                                JSON::Object node;
                                node[L"file"] = textureName;
                                if (mapSearch.first.compareNoCase(L"albedo") == 0)
                                {
                                    node[L"flags"] = L"sRGB";
                                }

                                dataNode.set(mapSearch.first, node);
                            }

                            std::function<void(const wchar_t *)> createParent;
                            createParent = [&](const wchar_t *directory) -> void
                            {
                                if (!FileSystem::IsDirectory(directory))
                                {
                                    createParent(FileSystem::GetDirectory(directory));
                                    CreateDirectory(directory, nullptr);
                                }
                            };

                            String materialPath(FileSystem::GetFileName(materialsPath, materialName).append(L".json"));
                            createParent(FileSystem::GetDirectory(materialPath));

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
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf(StringUTF8::Format("Caught: %v\r\nType: %v\r\n", exception.what(), typeid(exception).name()));
    }
    catch (...)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf("Caught: Non-standard exception\r\n");
    };

    printf("\r\n");
    return 0;
}