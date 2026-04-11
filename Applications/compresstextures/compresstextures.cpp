#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <DirectXTex.h>
#include <algorithm>
#include <chrono>
#include <locale>
#include <codecvt>
#include <string>
#include <map>

#include <argparse/argparse.hpp>

using namespace Gek;

enum class CompressStatus
{
	Skipped,
	Succeeded,
	Failed,
};

CompressStatus compressTexture(Context *context, FileSystem::Path const &inputFilePath)
{
	auto compressStartTime = std::chrono::steady_clock::now();

	if (!inputFilePath.isFile())
	{
		context->log(Context::Error, "Input file not found: {}", inputFilePath.getString());
		return CompressStatus::Failed;
	}

	auto outputFilePath(inputFilePath.withExtension(".dds"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		context->log(Context::Error, "Input file hasn't changed since last compression: {}", inputFilePath.getString());
		return CompressStatus::Skipped;
	}

	std::string extension(String::GetLower(inputFilePath.getExtension()));
	if (extension == ".dds")
	{
		context->log(Context::Error, "Input file is alrady compressed: {}", inputFilePath.getString());
		return CompressStatus::Skipped;
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
		return CompressStatus::Skipped;
	}

	std::vector<uint8_t> buffer(FileSystem::Load(inputFilePath));
	if (buffer.empty())
	{
		context->log(Context::Error, "Unable to read input file bytes: {}", inputFilePath.getString());
		return CompressStatus::Failed;
	}

	::DirectX::ScratchImage image;
	HRESULT resultValue = load(buffer, image);
	if (FAILED(resultValue))
	{
		context->log(
			Context::Error,
			"Unable to decode input file: {} (HRESULT=0x{:08X}, bytes={})",
			inputFilePath.getString(),
			static_cast<uint32_t>(resultValue),
			buffer.size());
		return CompressStatus::Failed;
	}

	auto flags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(0);
	DXGI_FORMAT outputFormat = DXGI_FORMAT_UNKNOWN;
	std::string textureName(String::GetLower(inputFilePath.withoutExtension().getString()));
	if (String::EndsWith(textureName, "basecolor") ||
		String::EndsWith(textureName, "base_color") ||
		String::EndsWith(textureName, "diffuse") ||
		String::EndsWith(textureName, "_diff") ||
		String::EndsWith(textureName, "_dif") ||
		String::EndsWith(textureName, "diffuse_s") ||
		String::EndsWith(textureName, "albedo") ||
		String::EndsWith(textureName, "albedo_s") ||
		String::EndsWith(textureName, "alb") ||
		String::EndsWith(textureName, "_d") ||
		String::EndsWith(textureName, "_c"))
	{
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
		String::EndsWith(textureName, "_ddn") ||
		String::EndsWith(textureName, "_nrm") ||
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
		return CompressStatus::Skipped;
	}
	else
	{
		context->log(Context::Error, "Unable to determine texture material type: {}", textureName);
        return CompressStatus::Skipped;
    }

	if (outputFormat == DXGI_FORMAT_BC7_UNORM_SRGB)
	{
		flags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(flags | ::DirectX::TEX_COMPRESS_BC7_QUICK);
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
	if (SUCCEEDED(resultValue))
	{
		image = std::move(mipMapChain);
	}
	else
	{
		context->log(Context::Warning, "Unable to create mipmap chain, compressing base level only");
	}

	// Ensure the compressor input matches the expected channel layout for BC targets.
	::DirectX::ScratchImage compressionInput;
	const ::DirectX::Image *compressionImages = image.GetImages();
	size_t compressionImageCount = image.GetImageCount();
	::DirectX::TexMetadata compressionMetadata = image.GetMetadata();
	DXGI_FORMAT requiredInputFormat = DXGI_FORMAT_UNKNOWN;
	if (outputFormat == DXGI_FORMAT_BC4_UNORM)
	{
		requiredInputFormat = DXGI_FORMAT_R8_UNORM;
	}
	else if (outputFormat == DXGI_FORMAT_BC5_UNORM)
	{
		requiredInputFormat = DXGI_FORMAT_R8G8_UNORM;
	}

	if (requiredInputFormat != DXGI_FORMAT_UNKNOWN && compressionMetadata.format != requiredInputFormat)
	{
		resultValue = ::DirectX::Convert(
			compressionImages,
			compressionImageCount,
			compressionMetadata,
			requiredInputFormat,
			::DirectX::TEX_FILTER_DEFAULT,
			::DirectX::TEX_THRESHOLD_DEFAULT,
			compressionInput);
		if (FAILED(resultValue))
		{
			context->log(Context::Error, "Unable to convert compressor input format (HRESULT=0x{:08X})", static_cast<uint32_t>(resultValue));
			return CompressStatus::Failed;
		}

		compressionImages = compressionInput.GetImages();
		compressionImageCount = compressionInput.GetImageCount();
		compressionMetadata = compressionInput.GetMetadata();
	}

	::DirectX::ScratchImage output;
	auto runCompressionPass = [&](::DirectX::TEX_COMPRESS_FLAGS activeFlags, char const *passName) -> HRESULT
	{
		context->log(Context::Info, "- {} compression started", passName);
		auto passStartTime = std::chrono::steady_clock::now();

		::DirectX::CompressOptions compressOptions{};
		compressOptions.flags = activeFlags;
		compressOptions.threshold = 0.5f;
		compressOptions.alphaWeight = 1.0f;

		auto lastHeartbeatTime = passStartTime;
		auto compressionResult = ::DirectX::CompressEx(
			compressionImages,
			compressionImageCount,
			compressionMetadata,
			outputFormat,
			compressOptions,
			output,
			[&](size_t completed, size_t total) -> bool
			{
				auto now = std::chrono::steady_clock::now();
				if ((now - lastHeartbeatTime) >= std::chrono::seconds(5))
				{
					lastHeartbeatTime = now;
					const double ratio = (total > 0 ? (static_cast<double>(completed) / static_cast<double>(total)) : 0.0);
					const auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - passStartTime).count();
					context->log(
						Context::Info,
						"- {} compression in progress: '{}' ({:.1f}% | {} s elapsed)",
						passName,
						inputFilePath.getString(),
						(ratio * 100.0),
						elapsedSeconds);
				}

				return true;
			});

		auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - passStartTime).count();
		context->log(
			Context::Info,
			"- {} compression finished in {} ms (HRESULT=0x{:08X})",
			passName,
			elapsedMilliseconds,
			static_cast<uint32_t>(compressionResult));
		return compressionResult;
	};

	resultValue = runCompressionPass(flags, "Primary");
	if (FAILED(resultValue))
	{
		// Retry once without parallel compression in case worker setup or memory pressure fails.
		auto fallbackFlags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(flags & ~::DirectX::TEX_COMPRESS_PARALLEL);
		resultValue = runCompressionPass(fallbackFlags, "Fallback");
	}

	if (FAILED(resultValue))
	{
		context->log(Context::Error, "Unable to compress image (HRESULT=0x{:08X})", static_cast<uint32_t>(resultValue));
        return CompressStatus::Failed;
    }

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	std::wstring wide = converter.from_bytes(outputFilePath.getString());
    resultValue = ::DirectX::SaveToDDSFile(output.GetImages(), output.GetImageCount(), output.GetMetadata(), ::DirectX::DDS_FLAGS_FORCE_DX10_EXT, wide.data());
	if (FAILED(resultValue))
	{
        context->log(Context::Info, "Unable to save image");
	return CompressStatus::Failed;
    }

	auto compressEndTime = std::chrono::steady_clock::now();
	auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(compressEndTime - compressStartTime).count();
	context->log(Context::Info, "Compressed '{}' in {} ms", inputFilePath.getString(), elapsedMilliseconds);
	return CompressStatus::Succeeded;
}

int main(int argumentCount, char const * const argumentList[])
{
	auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
	auto engineRootPath(pluginPath.getParentPath());
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

		const HRESULT comInitializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool comInitialized = SUCCEEDED(comInitializeResult) || comInitializeResult == RPC_E_CHANGED_MODE;
		if (!comInitialized)
		{
			context->log(Context::Error, "CoInitializeEx failed (HRESULT=0x{:08X})", static_cast<uint32_t>(comInitializeResult));
			return 1;
		}

		context->addDataPath(engineRootPath / "data");
		context->addDataPath(rootPath / "data");
		context->addDataPath(rootPath.getString());

		auto gekDataPath = std::getenv("gek_data_path");
		if (gekDataPath)
		{
			context->addDataPath(gekDataPath);
		}

		context->log(Context::Info, "Texture data roots: engine='{}' root='{}'", (engineRootPath / "data").getString(), (rootPath / "data").getString());
		if (gekDataPath)
		{
			context->log(Context::Info, "Texture data root override env='{}'", std::string(gekDataPath));
		}

			std::vector<FileSystem::Path> textureFileList;
			const std::vector<FileSystem::Path> candidateTextureRoots =
			{
				engineRootPath / "data" / "textures",
				rootPath / "data" / "textures",
				(gekDataPath ? (FileSystem::Path(gekDataPath) / "textures") : FileSystem::Path()),
			};

			std::vector<FileSystem::Path> activeTextureRoots;
			for (auto const &candidatePath : candidateTextureRoots)
			{
				if (!candidatePath.getString().empty() && candidatePath.isDirectory())
				{
					activeTextureRoots.push_back(candidatePath);
				}
			}

			if (activeTextureRoots.empty())
			{
				context->log(Context::Error, "No valid texture root directory found");
				return 1;
			}

			std::set<std::string> discoveredTextureSet;
			for (auto const &activeRoot : activeTextureRoots)
			{
				size_t rootFileCountBefore = textureFileList.size();
				context->log(Context::Info, "Scanning texture root: {}", activeRoot.getString());
				activeRoot.findFiles([&](FileSystem::Path const &filePath) -> bool
				{
					if (!filePath.isFile() || filePath.getExtension() == ".dds")
					{
						return true;
					}

					auto normalizedPath = String::GetLower(filePath.getString());
					if (discoveredTextureSet.insert(normalizedPath).second)
					{
						textureFileList.emplace_back(filePath.getString());
					}

					return true;
				});

				const size_t rootAddedCount = (textureFileList.size() - rootFileCountBefore);
				context->log(Context::Info, "- added {} files from {}", rootAddedCount, activeRoot.getString());
			}

			std::sort(std::begin(textureFileList), std::end(textureFileList), [](FileSystem::Path const &left, FileSystem::Path const &right) -> bool
			{
				return left.getString() < right.getString();
			});

			context->log(Context::Info, "Compression queue: {} textures", textureFileList.size());
			size_t succeededCount = 0;
			size_t skippedCount = 0;
			size_t failedCount = 0;
			auto batchStartTime = std::chrono::steady_clock::now();

			for (size_t textureIndex = 0; textureIndex < textureFileList.size(); ++textureIndex)
			{
				auto const &texturePath = textureFileList[textureIndex];
				context->log(Context::Info, "[{:5d}/{:5d}] {}", (textureIndex + 1), textureFileList.size(), texturePath.getString());

				auto status = compressTexture(context.get(), texturePath);
				switch (status)
				{
				case CompressStatus::Succeeded:
					++succeededCount;
					break;

				case CompressStatus::Skipped:
					++skippedCount;
					break;

				case CompressStatus::Failed:
					++failedCount;
					break;
				};

				const double completionRatio = (textureFileList.empty() ? 1.0 : (static_cast<double>(textureIndex + 1) / static_cast<double>(textureFileList.size())));
				context->log(
					Context::Info,
					"Progress: {:6.2f}% | ok={} skip={} fail={}",
					(completionRatio * 100.0),
					succeededCount,
					skippedCount,
					failedCount);
			}

			auto batchElapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - batchStartTime).count();
			context->log(
				Context::Info,
				"Compression finished: total={} ok={} skip={} fail={} elapsed={} ms",
				textureFileList.size(),
				succeededCount,
				skippedCount,
				failedCount,
				batchElapsedMilliseconds);

			if (SUCCEEDED(comInitializeResult))
			{
				CoUninitialize();
			}
	}

    return 0;
}