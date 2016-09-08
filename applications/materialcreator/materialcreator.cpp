#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
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

						Xml::Node materialNode = Xml::Node(L"material");
						Xml::Node &shaderNode = materialNode.getChild(L"shader");
						shaderNode.attributes[L"name"] = L"$standard";
						Xml::Node &solidNode = shaderNode.getChild(L"solid");

						FileSystem::find(setTexturePath, [&](const wchar_t *textureFileName) -> bool
						{
							String textureName(FileSystem::replaceExtension(textureFileName, L"").getLower());
							textureName.replace((texturesPath + L"\\"), L"");
							textureName.toLower();

							Xml::Node *node = nullptr;
							if (textureName.find(L"basecolor") != String::npos ||
								textureName.find(L"base_color") != String::npos ||
								textureName.find(L"diffuse") != String::npos ||
								textureName.find(L"albedo") != String::npos ||
								textureName.subString(textureName.length() - 2) == L"_d")
							{
								node = &solidNode.getChild(L"albedo");
								node->attributes[L"flags"] = L"sRGB";
							}
							else if (textureName.find(L"normal") != String::npos ||
								textureName.subString(textureName.length() - 2) == L"_n")
							{
								node = &solidNode.getChild(L"normal");
							}
							else if (textureName.find(L"roughness") != String::npos ||
								textureName.subString(textureName.length() - 2) == L"_r")
							{
								node = &solidNode.getChild(L"roughness");
							}
							else if (textureName.find(L"metalness") != String::npos ||
								textureName.find(L"metallic") != String::npos ||
								textureName.subString(textureName.length() - 2) == L"_m")
							{
								node = &solidNode.getChild(L"metallic");
							}

							if (node)
							{
								printf("--> %S: %S\r\n", node->type.c_str(), textureName.c_str());
								node->attributes[L"file"] = textureName;
							}

							return true;
						});

						if (solidNode.findChild(L"albedo", nullptr) &&
							solidNode.findChild(L"normal", nullptr))
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

							String materialPath(FileSystem::getFileName(materialsPath, materialName).append(L".xml"));
							createParent(FileSystem::getDirectory(materialPath));
							Xml::save(materialNode, materialPath);
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
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}