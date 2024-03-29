#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/RenderDevice.hpp"
#include "GEK/System/WindowDevice.hpp"
#include <DirectXTex.h>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <string>
#include <map>

#include <argparse/argparse.hpp>

using namespace Gek;

void compressTexture(Context *context, Render::Debug::Device *device, FileSystem::Path const &inputFilePath)
{
	if (!inputFilePath.isFile())
	{
		context->log(Context::Error, "Input file not found: {}", inputFilePath.getString());
		return;
	}

	auto outputFilePath(inputFilePath.withExtension(".dds"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		context->log(Context::Error, "Input file hasn't changed since last compression: {}", inputFilePath.getString());
		return;
	}

	std::string extension(String::GetLower(inputFilePath.getExtension()));
	if (extension == ".dds")
	{
		context->log(Context::Error, "Input file is alrady compressed: {}", inputFilePath.getString());
		return;
	}

	std::function<HRESULT(std::vector<uint8_t> const &, ::DirectX::ScratchImage &)> load;
	if (extension == ".tga")
	{
		load = [](std::vector<uint8_t> const &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
	}
    else if (extension == ".png" || extension == ".bmp" ||
             extension == ".jpg" || extension == ".jpeg" ||
             extension == ".tif" || extension == ".tiff")
    {
		load = [](std::vector<uint8_t> const &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_FLAGS_NONE, nullptr, image); };
	}
	/*
		else if (extension == ".dds")
		{
			load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, nullptr, std::placeholders::_3);
		}
	*/
	if (!load)
	{
		context->log(Context::Error, "Unknown file type of {} for input: {}", extension, inputFilePath.getString());
		return;
	}

	std::vector<uint8_t> buffer(FileSystem::Load(inputFilePath));

	::DirectX::ScratchImage image;
	HRESULT resultValue = load(buffer, image);
	if (FAILED(resultValue))
	{
		context->log(Context::Error, "Unable to load input file: {}", inputFilePath.getString());
		return;
	}

	bool useDevice = false;
	auto flags = ::DirectX::TEX_COMPRESS_PARALLEL;
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	std::string textureName(String::GetLower(inputFilePath.withoutExtension().getString()));
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
	else if (String::EndsWith(textureName, "clarity") ||
		String::EndsWith(textureName, "height") ||
		String::EndsWith(textureName, "occlusion") ||
		String::EndsWith(textureName, "thickness") ||
		String::EndsWith(textureName, "specular")
		)
	{
		context->log(Context::Error, "Skipping unhandled texture material type: {}", textureName);
		return;
	}
	else
	{
		context->log(Context::Error, "Unable to determine texture material type: {}", textureName);
        return;
    }

	context->log(Context::Info, "Compressing: {} to {}", inputFilePath.getString(), outputFilePath.getString());
    switch (outputFormat)
    {
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        context->log(Context::Info, "- Albedo BC7");
        break;

    case DXGI_FORMAT_BC1_UNORM_SRGB:
        context->log(Context::Info, "- Albedo BC1");
        break;

    case DXGI_FORMAT_BC5_UNORM:
        context->log(Context::Info, "- Normal BC5");
        break;

    case DXGI_FORMAT_BC4_UNORM:
        context->log(Context::Info, "- Metalness/Roughness BC4");
        break;
    };

    context->log(Context::Info, "- {} x {}", image.GetMetadata().width, image.GetMetadata().height);

	::DirectX::ScratchImage mipMapChain;
	resultValue = ::DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), ::DirectX::TEX_FILTER_TRIANGLE, 0, mipMapChain);
	if (FAILED(resultValue))
	{
		context->log(Context::Error, "Unable to create mipmap chain");
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
		context->log(Context::Error, "Unable to compress image");
        return;
    }

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(outputFilePath.getString());
    resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, wide.data());
	if (FAILED(resultValue))
	{
        context->log(Context::Info, "Unable to save image");
        return;
    }
}

int main(int argumentCount, char const * const argumentList[])
{
	auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
	auto cachePath(FileSystem::GetCacheFromModule());
	auto rootPath(cachePath.getParentPath());
	cachePath.setWorkingDirectory();

	std::vector<FileSystem::Path> searchPathList;
	searchPathList.push_back(pluginPath);

	ContextPtr context(Context::Create(&searchPathList));
	if (context)
	{
		context->log(Context::Info, "GEK Texture Compressor");
		context->setCachePath(cachePath);

		auto gekDataPath = std::getenv("gek_data_path");
		if (gekDataPath)
		{
			context->addDataPath(gekDataPath);
		}

		context->addDataPath(rootPath / "data");
		context->addDataPath(rootPath.getString());

		Window::Description description;
		description.className = "GEK_Engine_Textures";
		description.windowName = "GEK Engine Textures";
		Window::DevicePtr window(context->createClass<Window::Device>("Default::System::Window", description));

		Render::Device::Description deviceDescription;
		Render::DevicePtr device(context->createClass<Render::Device>("Default::Device::Video", window.get(), deviceDescription));
		if (device)
		{
			std::function<bool(FileSystem::Path const &)> searchDirectory;
			searchDirectory = [&](FileSystem::Path const &filePath) -> bool
			{
				if (filePath.isDirectory())
				{
					filePath.findFiles(searchDirectory);
				}
				else if (filePath.isFile() && filePath.getExtension() != ".dds")
				{
					compressTexture(context.get(), dynamic_cast<Render::Debug::Device *>(device.get()), String::GetLower(filePath.getString()));
				}

				return true;
			};

			CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
			context->findDataPath("textures"s).findFiles(searchDirectory);
			CoUninitialize();
		}
	}

    return 0;
}