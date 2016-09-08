#include "GEK\Math\Common.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Shapes\AlignedBox.h"
#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Utility\XML.h"
#include <Shlwapi.h>
#include <DirectXTex.h>
#include <experimental\filesystem>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

using namespace Gek;

void compressTexture(const String &inputFileName)
{
	if (!FileSystem::isFile(inputFileName))
	{
		throw std::exception("Input file not found");
	}

	String outputFileName(FileSystem::replaceExtension(inputFileName, L".dds"));
	if (FileSystem::isFile(outputFileName))
	{
		throw std::exception("Output already exists (must specify overwrite)");
	}

	printf("Compressing: -> %S\r\n", inputFileName.c_str());
	printf("             <- %S\r\n", outputFileName.c_str());

	std::vector<uint8_t> buffer;
	FileSystem::load(inputFileName, buffer);

	String extension(FileSystem::getExtension(inputFileName));
	std::function<HRESULT(const std::vector<uint8_t> &, ::DirectX::ScratchImage &)> load;
	if (extension.compareNoCase(L".tga") == 0)
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
	}
	else if (extension.compareNoCase(L".png") == 0)
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_PNG, nullptr, image); };
	}
	else if (extension.compareNoCase(L".bmp") == 0)
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_BMP, nullptr, image); };
	}
	else if (extension.compareNoCase(L".jpg") == 0 ||
		extension.compareNoCase(L".jpeg") == 0)
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_JPEG, nullptr, image); };
	}
/*
	else if (extension.compareNoCase(L".dds") == 0)
	{
		load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, nullptr, std::placeholders::_3);
	}
*/
	if (!load)
	{
		throw std::exception("Unknown file type listed for input");
	}

	::DirectX::ScratchImage image;
	HRESULT resultValue = load(buffer, image);
	if (FAILED(resultValue))
	{
		throw std::exception("Unable to load input file");
	}

	uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	if (inputFileName.find(L"basecolor") != String::npos ||
		inputFileName.find(L"base_color") != String::npos ||
		inputFileName.find(L"diffuse") != String::npos ||
		inputFileName.find(L"albedo") != String::npos ||
		inputFileName.subString(inputFileName.length() - 2) == L"_d")
	{
		flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
		flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
		if (DirectX::HasAlpha(image.GetMetadata().format))
		{
			outputFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
			printf("Compressing Albedo: BC7 sRGB\r\n");
		}
		else
		{
			outputFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
			printf("Compressing Albedo: BC7 sRGB\r\n");
		}
	}
	else if (inputFileName.find(L"normal") != String::npos ||
		inputFileName.subString(inputFileName.length() - 2) == L"_n")
	{
		outputFormat = DXGI_FORMAT_BC5_UNORM;
		printf("Compressing Normal: BC5\r\n");
	}
	else if (inputFileName.find(L"roughness") != String::npos ||
		inputFileName.subString(inputFileName.length() - 2) == L"_r")
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
		printf("Compressing Roughness: BC4\r\n");
	}
	else if (inputFileName.find(L"metalness") != String::npos ||
		inputFileName.find(L"metallic") != String::npos ||
		inputFileName.subString(inputFileName.length() - 2) == L"_m")
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
		printf("Compressing Metallic: BC4\r\n");
	}
	else
	{
		throw std::exception("Unknown material encountered");
	}

	printf(".loaded.");

	::DirectX::ScratchImage mipMapChain;
	resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
	if (FAILED(resultValue))
	{
		throw std::exception("Unabole to create mipmap chain");
	}

	image = std::move(mipMapChain);
	printf(".mipmapped.");

	::DirectX::ScratchImage output;
	resultValue = ::DirectX::Compress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), outputFormat, flags, 0.5f, output);
	if (FAILED(resultValue))
	{
		throw std::exception("Unable to compress image");
	}

	printf(".compressed.");

	resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, outputFileName);
	if (FAILED(resultValue))
	{
		throw std::exception("Unable to save image");
	}

	printf(".done!\r\n");
}

int wmain(int argumentCount, const wchar_t *argumentList[], const wchar_t *environmentVariableList)
{
    printf("GEK Texture Compressor\r\n");

    try
    {
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

		std::function<bool(const wchar_t *)> searchDirectory;
		searchDirectory = [&](const wchar_t *fileName) -> bool
		{
			if (FileSystem::isDirectory(fileName))
			{
				FileSystem::find(fileName, searchDirectory);
			}
			else if (FileSystem::isFile(fileName))
			{
				try
				{
					compressTexture(String(fileName).getLower());
				}
				catch (...)
				{
					printf("[warning] Error trying to compress file: %S\r\n", fileName);
				};
			}

			return true;
		};

		FileSystem::find(texturesPath, searchDirectory);
    }
    catch (const std::exception &exception)
    {
        printf("[error] Exception occurred: %s", exception.what());
    }
    catch (...)
    {
        printf("\r\n[error] Unhandled Exception Occurred!");
    }

    printf("\r\n");
    return 0;
}