#include "GEK\Math\Constants.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\SIMD4x4.hpp"
#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\Utility\Context.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include <Shlwapi.h>
#include <DirectXTex.h>
#include <experimental\filesystem>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <map>

using namespace Gek;

void compressTexture(Video::Debug::Device *device, const String &inputFileName)
{
	if (!FileSystem::isFile(inputFileName))
	{
		throw std::exception("Input file not found");
	}

	String outputFileName(FileSystem::replaceExtension(inputFileName, L".dds"));
	if (FileSystem::isFile(outputFileName) && FileSystem::isFileNewer(outputFileName, inputFileName))
	{
		throw std::exception("Input file hasn't changed since last compression");
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

	bool useDevice = false;
	String inputName(FileSystem::replaceExtension(inputFileName));
	uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	if (inputName.endsWith(L"basecolor") ||
		inputName.endsWith(L"base_color") ||
		inputName.endsWith(L"diffuse") ||
		inputName.endsWith(L"albedo") ||
		inputName.endsWith(L"alb") ||
		inputName.endsWith(L"_d"))
	{
		useDevice = true;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
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
	else if (inputName.endsWith(L"normal") ||
		inputName.endsWith(L"_n"))
	{
		outputFormat = DXGI_FORMAT_BC5_UNORM;
		printf("Compressing Normal: BC5\r\n");
	}
	else if (inputName.endsWith(L"roughness") ||
		inputName.endsWith(L"rough") ||
		inputName.endsWith(L"_r"))
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
		printf("Compressing Roughness: BC4\r\n");
	}
	else if (inputName.endsWith(L"metalness") ||
		inputName.endsWith(L"metallic") ||
		inputName.endsWith(L"metal") ||
		inputName.endsWith(L"_m"))
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
	if(useDevice)
	{
		// Hardware accelerated compression only works with BC7 format
		resultValue = ::DirectX::Compress((ID3D11Device *)device->getDevice(), image.GetImages(), image.GetImageCount(), image.GetMetadata(), outputFormat, flags, 0.5f, output);
	}
	else
	{
		resultValue = ::DirectX::Compress(image.GetImages(), image.GetImageCount(), image.GetMetadata(), outputFormat, flags, 0.5f, output);
	}

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

		rootPath = fullModulePath.wstring();

		std::vector<String> searchPathList;

#ifdef _DEBUG
		SetCurrentDirectory(FileSystem::getFileName(rootPath, L"Debug"));
		searchPathList.push_back(FileSystem::getFileName(rootPath, L"Debug\\Plugins"));
#else
		SetCurrentDirectory(FileSystem::getFileName(rootPath, L"Release"));
		searchPathList.push_back(FileSystem::getFileName(rootPath, L"Release\\Plugins"));
#endif

		String texturesPath(FileSystem::getFileName(rootPath, L"Data\\Textures").getLower());

		WNDCLASS windowClass;
		windowClass.style = 0;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(103));
		windowClass.hCursor = nullptr;
		windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = L"GEKvX_Engine_Texture_Compressor";
		ATOM classAtom = RegisterClass(&windowClass);
		if (!classAtom)
		{
			throw std::exception("Unable to register window class");
		}

		HWND window = CreateWindow(L"GEKvX_Engine_Texture_Compressor", L"GEKvX Application - Texture Compressor", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, 0, 0, 1, 1, 0, nullptr, GetModuleHandle(nullptr), 0);
		if (window == nullptr)
		{
			throw std::exception("Unable to create window");
		}

		ContextPtr context(Context::create(rootPath, searchPathList));
		Video::DevicePtr device(context->createClass<Video::Device>(L"Device::Video::D3D11", window, Video::Format::R8G8B8A8_UNORM_SRGB, String(L"default")));

		std::function<bool(const wchar_t *)> searchDirectory;
		searchDirectory = [&](const wchar_t *fileName) -> bool
		{
			if (FileSystem::isDirectory(fileName))
			{
				FileSystem::find(fileName, searchDirectory);
			}
			else if (FileSystem::isFile(fileName) && FileSystem::getExtension(fileName).compareNoCase(L".dds") != 0)
			{
				try
				{
					compressTexture(dynamic_cast<Video::Debug::Device *>(device.get()), String(fileName).getLower());
				}
				catch (...)
				{
					printf("[warning] Error trying to compress file: %S\r\n", fileName);
				};
			}

			return true;
		};

		CoInitialize(nullptr);
		FileSystem::find(texturesPath, searchDirectory);
		CoUninitialize();
	
		DestroyWindow(window);
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