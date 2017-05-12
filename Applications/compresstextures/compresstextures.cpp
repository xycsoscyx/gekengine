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

void compressTexture(Video::Debug::Device *device, FileSystem::Path const &inputFilePath)
{
	if (!inputFilePath.isFile())
	{
		throw std::exception("Input file not found");
	}

    auto outputFilePath(inputFilePath.withExtension(".dds"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		throw std::exception("Input file hasn't changed since last compression");
	}

	std::cout << "Compressing: -> " << inputFilePath << std::endl;
	std::cout << "             <- " << outputFilePath << std::endl;

	static const std::vector<uint8_t> EmptyBuffer;
	std::vector<uint8_t> buffer(FileSystem::Load(inputFilePath, EmptyBuffer));

	std::string extension(String::GetLower(inputFilePath.getExtension()));
	std::function<HRESULT(const std::vector<uint8_t> &, ::DirectX::ScratchImage &)> load;
	if (extension == ".tga")
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
	}
	else if (extension == ".png")
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_PNG, nullptr, image); };
	}
	else if (extension == ".bmp")
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_BMP, nullptr, image); };
	}
	else if (extension == ".jpg" || extension == ".jpeg")
	{
		load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_JPEG, nullptr, image); };
	}
/*
	else if (extension == ".dds")
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
	uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	std::string inputName(String::GetLower(inputFilePath.withoutExtension().u8string()));
	if (String::EndsWith(inputName, "basecolor") ||
		String::EndsWith(inputName, "base_color") ||
		String::EndsWith(inputName, "diffuse") ||
		String::EndsWith(inputName, "albedo") ||
		String::EndsWith(inputName, "alb") ||
        String::EndsWith(inputName, "_d") ||
        String::EndsWith(inputName, "_c"))
{
		useDevice = true;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
		if (DirectX::HasAlpha(image.GetMetadata().format))
		{
			outputFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
			std::cout << "Compressing Albedo: BC7 sRGB" << std::endl;
		}
		else
		{
			outputFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
			std::cout << "Compressing Albedo: BC7 sRGB" << std::endl;
		}
	}
	else if (String::EndsWith(inputName, "normal") ||
		String::EndsWith(inputName, "_n"))
	{
		outputFormat = DXGI_FORMAT_BC5_UNORM;
		std::cout << "Compressing Normal: BC5" << std::endl;
	}
	else if (String::EndsWith(inputName, "roughness") ||
		String::EndsWith(inputName, "rough") ||
		String::EndsWith(inputName, "_r"))
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
		std::cout << "Compressing Roughness: BC4" << std::endl;
	}
	else if (String::EndsWith(inputName, "metalness") ||
		String::EndsWith(inputName, "metallic") ||
		String::EndsWith(inputName, "metal") ||
		String::EndsWith(inputName, "_m"))
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
		std::cout << "Compressing Metallic: BC4" << std::endl;
	}
	else
	{
		throw std::exception("Unknown material encountered");
	}

	std::cout << ".loaded.";

	::DirectX::ScratchImage mipMapChain;
	resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
	if (FAILED(resultValue))
	{
		throw std::exception("Unabole to create mipmap chain");
	}

	image = std::move(mipMapChain);
	std::cout << ".mipmapped.";

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

	std::cout << ".compressed.";

	resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, outputFilePath.c_str());
	if (FAILED(resultValue))
	{
		throw std::exception("Unable to save image");
	}

	std::cout << ".done!" << std::endl;
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    std::cout << "GEK Texture Compressor" << std::endl;

    try
    {
        auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
        std::vector<FileSystem::Path> searchPathList;
        searchPathList.push_back(pluginPath);

        auto rootPath(pluginPath.getParentPath());
        ContextPtr context(Context::Create(rootPath, searchPathList));

        Window::Description description;
        description.className = "GEK_Engine_Textures";
        description.windowName = "GEK Engine Textures";
        WindowPtr window(context->createClass<Window>("Default::System::Window", description));

        Video::Device::Description deviceDescription;
        Video::DevicePtr device(context->createClass<Video::Device>("Device::Video::D3D11", window.get(), deviceDescription));

        std::function<bool(FileSystem::Path const &)> searchDirectory;
		searchDirectory = [&](FileSystem::Path const &filePath) -> bool
		{
			if (filePath.isDirectory())
			{
				FileSystem::Find(filePath, searchDirectory);
			}
			else if (filePath.isFile() && filePath.getExtension() != ".dds")
			{
				try
				{
					compressTexture(dynamic_cast<Video::Debug::Device *>(device.get()), String::GetLower(filePath.u8string()));
				}
				catch (...)
				{
					std::cerr << "[warning] Error trying to compress file: " << filePath << std::endl;
				};
			}

			return true;
		};

		CoInitialize(nullptr);
		FileSystem::Find(FileSystem::GetFileName(rootPath, "Data", "Textures"), searchDirectory);
		CoUninitialize();
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