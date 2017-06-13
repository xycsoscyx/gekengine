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
        LockedWrite{ std::cerr } << String::Format("Input file not found: %v", inputFilePath.c_str());
        return;
	}

    auto outputFilePath(inputFilePath.withExtension(".dds"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		LockedWrite{ std::cerr } << String::Format("Input file hasn't changed since last compression: %v", inputFilePath.c_str());
        return;
    }

    std::string extension(String::GetLower(inputFilePath.getExtension()));
    if (extension == ".dds")
    {
        LockedWrite{ std::cerr } << String::Format("Input file is alrady compressed: %v", inputFilePath.c_str());
        return;
    }

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
    else if (extension == ".tif" || extension == ".tiff")
    {
        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_TIFF, nullptr, image); };
    }
/*
	else if (extension == ".dds")
	{
		load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, nullptr, std::placeholders::_3);
	}
*/
	if (!load)
	{
        LockedWrite{ std::cerr } << String::Format("Unknown file type of %v for input: %v", extension, inputFilePath.c_str());
        return;
    }

    static const std::vector<uint8_t> EmptyBuffer;
    std::vector<uint8_t> buffer(FileSystem::Load(inputFilePath, EmptyBuffer));

    ::DirectX::ScratchImage image;
    HRESULT resultValue = load(buffer, image);
    if (FAILED(resultValue))
    {
        LockedWrite{ std::cerr } << String::Format("Unable to load input file: %v", inputFilePath.c_str());
        return;
    }

    bool useDevice = false;
	uint32_t flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	std::string textureName(String::GetLower(inputFilePath.withoutExtension().u8string()));
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
		useDevice = true;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_IN;
		//flags |= ::DirectX::TEX_COMPRESS_SRGB_OUT;
		if (DirectX::HasAlpha(image.GetMetadata().format))
		{
			outputFormat = DXGI_FORMAT_BC7_UNORM_SRGB;
		}
		else
		{
			outputFormat = DXGI_FORMAT_BC1_UNORM_SRGB;
		}
	}
    else if (String::EndsWith(textureName, "normal") ||
        String::EndsWith(textureName, "normalmap") ||
        String::EndsWith(textureName, "normalmap_s") ||
        String::EndsWith(textureName, "_n"))
    {
		outputFormat = DXGI_FORMAT_BC5_UNORM;
	}
    else if (String::EndsWith(textureName, "roughness") ||
        String::EndsWith(textureName, "roughness_s") ||
        String::EndsWith(textureName, "rough") ||
        String::EndsWith(textureName, "_r"))
    {
		outputFormat = DXGI_FORMAT_BC4_UNORM;
	}
	else if (String::EndsWith(textureName, "metalness") ||
		String::EndsWith(textureName, "metallic") ||
		String::EndsWith(textureName, "metal") ||
		String::EndsWith(textureName, "_m"))
	{
		outputFormat = DXGI_FORMAT_BC4_UNORM;
	}
	else
	{
		LockedWrite{ std::cerr } << String::Format("Unable to determine texture material type: %v", textureName);
        return;
    }

    LockedWrite{ std::cout } << String::Format("Compressing: -> %v", inputFilePath);
    LockedWrite{ std::cout } << String::Format("             <- %v", outputFilePath);
    switch (outputFormat)
    {
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        LockedWrite{ std::cout } << String::Format("             Albedo BC7");
        break;

    case DXGI_FORMAT_BC1_UNORM_SRGB:
        LockedWrite{ std::cout } << String::Format("             Albedo BC1");
        break;

    case DXGI_FORMAT_BC5_UNORM:
        LockedWrite{ std::cout } << String::Format("             Normal BC5");
        break;

    case DXGI_FORMAT_BC4_UNORM:
        LockedWrite{ std::cout } << String::Format("             Metalness/Roughness BC4");
        break;
    };

    LockedWrite{ std::cout } << String::Format("             %vx%v", image.GetMetadata().width, image.GetMetadata().height);

	::DirectX::ScratchImage mipMapChain;
	resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
	if (FAILED(resultValue))
	{
		LockedWrite{ std::cerr } << String::Format("Unable to create mipmap chain");
	}

	image = std::move(mipMapChain);

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
		LockedWrite{ std::cerr } << String::Format("Unable to compress image");
        return;
    }

	resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, outputFilePath.c_str());
	if (FAILED(resultValue))
	{
		LockedWrite{ std::cerr } << String::Format("Unable to save image");
        return;
    }
}

int wmain(int argumentCount, wchar_t const * const argumentList[], wchar_t const * const environmentVariableList)
{
    LockedWrite{ std::cout } << String::Format("GEK Texture Compressor");

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
        Video::DevicePtr device(context->createClass<Video::Device>("Default::Device::Video", window.get(), deviceDescription));

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
				catch (std::exception const &exception)
				{
                    LockedWrite{ std::cerr } << String::Format("[warning] Error trying to compress file: %v", filePath);
                    LockedWrite{ std::cerr } << String::Format(exception.what());
				}
                catch (...)
                {
                    LockedWrite{ std::cerr } << String::Format("[warning] Unknown error trying to compress file: %v", filePath);
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
        LockedWrite{ std::cerr } << String::Format("GEK Engine - Error");
		LockedWrite{ std::cerr } << String::Format("Caught: %v", exception.what());
		LockedWrite{ std::cerr } << String::Format("Type: %v", typeid(exception).name());
    }
    catch (...)
    {
        LockedWrite{ std::cerr } << String::Format("GEK Engine - Error");
        LockedWrite{ std::cerr } << String::Format("Caught: Non-standard exception");
    };

    return 0;
}