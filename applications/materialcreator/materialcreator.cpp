#include "GEK\Math\Common.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include <Windows.h>
#include <experimental\filesystem>

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

		String texturesPath(FileSystem::getFileName(rootPath, L"Textures").getLower());
		String materialsPath(FileSystem::getFileName(rootPath, L"Materials").getLower());

		std::function<bool(const wchar_t *)> findMaterials;
		findMaterials = [&](const wchar_t *setDirectoryName) -> bool
		{
			if (FileSystem::isDirectory(setDirectoryName))
			{
				printf("Set Found: %S\r\n", setDirectoryName);
				FileSystem::find(setDirectoryName, [&](const wchar_t *setTexturePath) -> bool
				{
					if (FileSystem::isDirectory(setTexturePath))
					{
						String materialName(setTexturePath);
						materialName.replace((texturesPath + L"\\"), L"");
						printf("> Material Found: %S\r\n", materialName.c_str());

						JSON::Object solidNode;
						FileSystem::find(setTexturePath, [&](const wchar_t *textureFileName) -> bool
						{
							String textureName(FileSystem::replaceExtension(textureFileName).getLower());
							textureName.replace((texturesPath + L"\\"), L"");
							textureName.toLower();

							String mapType;
							JSON::Object node;
							if (textureName.endsWith(L"basecolor") ||
								textureName.endsWith(L"base_color") ||
								textureName.endsWith(L"diffuse") ||
								textureName.endsWith(L"albedo") ||
								textureName.endsWith(L"alb") ||
								textureName.endsWith(L"_d"))
							{
								mapType = L"albedo";
								node[L"flags"] = L"sRGB";
							}
							else if (textureName.endsWith(L"normal") ||
								textureName.endsWith(L"_n"))
							{
								mapType = L"normal";
							}
							else if (textureName.endsWith(L"roughness") ||
								textureName.endsWith(L"rough") ||
								textureName.endsWith(L"_r"))
							{
								mapType = L"roughness";
							}
							else if (textureName.endsWith(L"metalness") ||
								textureName.endsWith(L"metallic") ||
								textureName.endsWith(L"metal") ||
								textureName.endsWith(L"_m"))
							{
								mapType = L"metallic";
							}

							if (!mapType.empty())
							{
								node[L"file"] = textureName;
								solidNode.set(mapType, node);
							}

							return true;
						});

						if (solidNode.get(L"albedo").is_object() &&
							solidNode.get(L"normal").is_object())
						{
							std::function<void(const wchar_t *)> createParent;
							createParent = [&](const wchar_t *directory) -> void
							{
								if (!FileSystem::isDirectory(directory))
								{
									createParent(FileSystem::getDirectory(directory));
									CreateDirectory(directory, nullptr);
								}
							};

							String materialPath(FileSystem::getFileName(materialsPath, materialName).append(L".json"));
							createParent(FileSystem::getDirectory(materialPath));

							JSON::Object shaderNode;
							shaderNode.set(L"solid", solidNode);

                            JSON::Object passesNode;
                            passesNode.set(L"passes", shaderNode);
                            passesNode.set(L"name", L"solid");

							JSON::Object materialNode;
							materialNode.set(L"shader", passesNode);
                            JSON::save(materialPath, materialNode);
						}
					}

					return true;
				});
			}

			return true;
		};

		FileSystem::find(texturesPath, findMaterials);
    }
    catch (const std::exception &exception)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf(StringUTF8::create("Caught: %v\r\nType: %v\r\n", exception.what(), typeid(exception).name()));
    }
    catch (...)
    {
        printf("\r\n\r\nGEK Engine - Error\r\n");
        printf("Caught: Non-standard exception\r\n");
    };

    printf("\r\n");
    return 0;
}