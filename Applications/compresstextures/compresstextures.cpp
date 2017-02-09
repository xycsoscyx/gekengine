#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/System/Window.hpp"
#include <algorithm>
#include <map>

#include <DirectXTex.h>

using namespace Gek;

void compressTexture(Video::Debug::Device *device, const FileSystem::Path &inputFilePath)
{
	if (!inputFilePath.isFile())
	{
		throw std::exception("Input file not found");
	}

    auto outputFilePath(inputFilePath.withExtension(L".dds"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		throw std::exception("Input file hasn't changed since last compression");
	}

	printf("Compressing: -> %S\r\n", inputFilePath.c_str());
	printf("             <- %S\r\n", outputFilePath.c_str());

	std::vector<uint8_t> buffer;
	FileSystem::Load(inputFilePath, buffer);

	String extension(inputFilePath.getExtension());
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

    String inputName(inputFilePath.withoutExtension());
    inputName.toLower();
    
    bool useDevice = false;
	uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	if (inputName.endsWith(L"basecolor") ||
		inputName.endsWith(L"base_color") ||
		inputName.endsWith(L"diffuse") ||
		inputName.endsWith(L"albedo") ||
		inputName.endsWith(L"alb") ||
        inputName.endsWith(L"_d") ||
        inputName.endsWith(L"_C"))
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

	resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, outputFilePath);
	if (FAILED(resultValue))
	{
		throw std::exception("Unable to save image");
	}

	printf(".done!\r\n");
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    printf("GEK Texture Compressor\r\n");

    try
    {
        auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
        std::vector<FileSystem::Path> searchPathList;
        searchPathList.push_back(pluginPath);

        auto rootPath(pluginPath.getParentPath());
        ContextPtr context(Context::Create(rootPath, searchPathList));

        Window::Description description;
        description.className = L"GEK_Engine_Textures";
        description.windowName = L"GEK Engine Textures";
        WindowPtr window(context->createClass<Window>(L"Default::System::Window", description));

        Video::Device::Description deviceDescription;
        Video::DevicePtr device(context->createClass<Video::Device>(L"Device::Video::D3D11", window.get(), deviceDescription));

        std::function<bool(wchar_t const * const )> searchDirectory;
		searchDirectory = [&](const FileSystem::Path &filePath) -> bool
		{
			if (filePath.isDirectory())
			{
				FileSystem::Find(filePath, searchDirectory);
			}
			else if (filePath.isFile() && filePath.getExtension().compareNoCase(L".dds") != 0)
			{
				try
				{
					compressTexture(dynamic_cast<Video::Debug::Device *>(device.get()), String(filePath).getLower());
				}
				catch (...)
				{
					printf("[warning] Error trying to compress file: %S\r\n", filePath.c_str());
				};
			}

			return true;
		};

		CoInitialize(nullptr);
		FileSystem::Find(FileSystem::GetFileName(rootPath, L"Data", L"Textures"), searchDirectory);
		CoUninitialize();
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