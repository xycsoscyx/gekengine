#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include <ktx.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <locale>
#include <codecvt>
#include <string>
#include <map>

#if defined(_WIN32)
#include <d3d11.h>
#endif

#include <argparse/argparse.hpp>

using namespace Gek;

enum class CompressStatus
{
	Skipped,
	Succeeded,
	Failed,
};

struct CompressionSettings
{
	bool enableParallel = true;
	bool enableGPU = true;
};

#if defined(_WIN32)
struct CompressionDevice
{
	ID3D11Device *device = nullptr;

	~CompressionDevice()
	{
		if (device)
		{
			device->Release();
			device = nullptr;
		}
	}
};

bool IsTruthyEnv(char const *value)
{
	if (!value)
	{
		return false;
	}

	std::string text = String::GetLower(value);
	return (text == "1" || text == "true" || text == "yes" || text == "on");
}

void InitializeCompressionDevice(Context *context, CompressionSettings const &settings, CompressionDevice &compressionDevice)
{
	if (!settings.enableGPU)
	{
		context->log(Context::Info, "GPU compression disabled by configuration");
		return;
	}

	static constexpr D3D_FEATURE_LEVEL featureLevelList[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	D3D_FEATURE_LEVEL chosenFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	HRESULT result = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0,
		featureLevelList,
		static_cast<UINT>(std::size(featureLevelList)),
		D3D11_SDK_VERSION,
		&compressionDevice.device,
		&chosenFeatureLevel,
		nullptr);

	if (FAILED(result) || !compressionDevice.device)
	{
		context->log(
			Context::Warning,
			"Unable to create D3D11 hardware device for GPU compression (HRESULT=0x{:08X}); CPU path will be used",
			static_cast<uint32_t>(result));
		compressionDevice.device = nullptr;
		return;
	}

	context->log(Context::Info, "GPU compression device initialized (feature level=0x{:04X})", static_cast<uint32_t>(chosenFeatureLevel));
}
#endif

bool inspectTexture(Context *context, FileSystem::Path const &inputFilePath)
{
	if (!inputFilePath.isFile())
	{
		context->log(Context::Error, "Texture file not found: {}", inputFilePath.getString());
		return false;
	}

	auto sourceData = FileSystem::Load(inputFilePath);
	if (sourceData.empty())
	{
		context->log(Context::Error, "Unable to read DDS file bytes: {}", inputFilePath.getString());
		return false;
	}

	const std::string extension = String::GetLower(inputFilePath.getExtension());
	const bool isDDS = (extension == ".dds");

	::DirectX::ScratchImage decodedImage;
	HRESULT resultValue = E_FAIL;
	if (isDDS)
	{
		resultValue = ::DirectX::LoadFromDDSMemory(sourceData.data(), sourceData.size(), ::DirectX::DDS_FLAGS_NONE, nullptr, decodedImage);
	}
	else if (extension == ".tga")
	{
		resultValue = ::DirectX::LoadFromTGAMemory(sourceData.data(), sourceData.size(), nullptr, decodedImage);
	}
	else if (extension == ".png" || extension == ".bmp" || extension == ".jpg" || extension == ".jpeg" || extension == ".tif" || extension == ".tiff")
	{
		resultValue = ::DirectX::LoadFromWICMemory(sourceData.data(), sourceData.size(), ::DirectX::WIC_FLAGS_NONE, nullptr, decodedImage);
	}
	else
	{
		context->log(Context::Error, "Unsupported file extension for inspection: {}", inputFilePath.getString());
		return false;
	}

	if (FAILED(resultValue))
	{
		context->log(Context::Error, "Unable to decode texture file: {} (HRESULT=0x{:08X})", inputFilePath.getString(), static_cast<uint32_t>(resultValue));
		return false;
	}

	const auto metadata = decodedImage.GetMetadata();
	context->log(Context::Info, "Inspecting texture: {}", inputFilePath.getString());
	context->log(Context::Info, "- metadata: {}x{} mip={} format={}", metadata.width, metadata.height, metadata.mipLevels, static_cast<uint32_t>(metadata.format));
	context->log(Context::Info, "- source type: {}", (isDDS ? "dds" : "source-image"));

	std::string textureName(String::GetLower(inputFilePath.withoutExtension().getString()));
	const bool nameSuggestsNormal =
		String::EndsWith(textureName, "normal") ||
		String::EndsWith(textureName, "normalmap") ||
		String::EndsWith(textureName, "normalmap_s") ||
		String::EndsWith(textureName, "_ddn") ||
		String::EndsWith(textureName, "_nrm") ||
		String::EndsWith(textureName, "_n");

	const bool isLikelyNormalEncoding =
		(metadata.format == DXGI_FORMAT_BC5_UNORM) ||
		(metadata.format == DXGI_FORMAT_BC5_SNORM) ||
		(metadata.format == DXGI_FORMAT_R8G8_UNORM) ||
		(metadata.format == DXGI_FORMAT_R8G8_SNORM) ||
		nameSuggestsNormal;

	if (!isLikelyNormalEncoding)
	{
		context->log(Context::Warning, "- format is not a typical normal map encoding (BC5/RG8), skipping normal-vector interpretation");
	}

	::DirectX::ScratchImage linearFloat;
	::DirectX::ScratchImage expandedImage;
	const ::DirectX::Image *conversionImages = decodedImage.GetImages();
	size_t conversionImageCount = decodedImage.GetImageCount();
	::DirectX::TexMetadata conversionMetadata = metadata;

	if (::DirectX::IsCompressed(metadata.format))
	{
		resultValue = ::DirectX::Decompress(
			decodedImage.GetImages(),
			decodedImage.GetImageCount(),
			metadata,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			expandedImage);
		if (FAILED(resultValue))
		{
			context->log(Context::Error, "Unable to decompress DDS for inspection (HRESULT=0x{:08X})", static_cast<uint32_t>(resultValue));
			return false;
		}

		conversionImages = expandedImage.GetImages();
		conversionImageCount = expandedImage.GetImageCount();
		conversionMetadata = expandedImage.GetMetadata();
	}

	resultValue = ::DirectX::Convert(
		conversionImages,
		conversionImageCount,
		conversionMetadata,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		::DirectX::TEX_FILTER_DEFAULT,
		::DirectX::TEX_THRESHOLD_DEFAULT,
		linearFloat);
	if (FAILED(resultValue))
	{
		context->log(Context::Error, "Unable to convert DDS to RGBA32F for inspection (HRESULT=0x{:08X})", static_cast<uint32_t>(resultValue));
		return false;
	}

	const ::DirectX::Image *image = linearFloat.GetImage(0, 0, 0);
	if (!image || !image->pixels)
	{
		context->log(Context::Error, "DDS image has no accessible pixels: {}", inputFilePath.getString());
		return false;
	}

	const size_t width = image->width;
	const size_t height = image->height;

	float minR = 1.0f;
	float minG = 1.0f;
	float minB = 1.0f;
	float minA = 1.0f;
	float maxR = 0.0f;
	float maxG = 0.0f;
	float maxB = 0.0f;
	float maxA = 0.0f;

	for (size_t y = 0; y < height; ++y)
	{
		auto row = reinterpret_cast<float const *>(image->pixels + (image->rowPitch * y));
		for (size_t x = 0; x < width; ++x)
		{
			auto const r = row[(x * 4) + 0];
			auto const g = row[(x * 4) + 1];
			auto const b = row[(x * 4) + 2];
			auto const a = row[(x * 4) + 3];
			minR = std::min(minR, r);
			minG = std::min(minG, g);
			minB = std::min(minB, b);
			minA = std::min(minA, a);
			maxR = std::max(maxR, r);
			maxG = std::max(maxG, g);
			maxB = std::max(maxB, b);
			maxA = std::max(maxA, a);
		}
	}

	context->log(Context::Info, "- channel range: R=[{:.6f}, {:.6f}] G=[{:.6f}, {:.6f}] B=[{:.6f}, {:.6f}] A=[{:.6f}, {:.6f}]", minR, maxR, minG, maxG, minB, maxB, minA, maxA);

	auto logSample = [&](size_t x, size_t y)
	{
		auto row = reinterpret_cast<float const *>(image->pixels + (image->rowPitch * y));
		auto const r = row[(x * 4) + 0];
		auto const g = row[(x * 4) + 1];
		auto const b = row[(x * 4) + 2];
		auto const a = row[(x * 4) + 3];

		if (!isLikelyNormalEncoding)
		{
			context->log(
				Context::Info,
				"- sample ({}, {}): RGBA=({:.6f}, {:.6f}, {:.6f}, {:.6f})",
				x,
				y,
				r,
				g,
				b,
				a);
			return;
		}

		const float nxRG = (r * 2.0f) - 1.0f;
		const float nyRG = (g * 2.0f) - 1.0f;
		const float nzRG = std::sqrt(std::max(0.0f, 1.0f - (nxRG * nxRG) - (nyRG * nyRG)));
		const float nyRGFlip = -nyRG;
		const float nzRGFlip = std::sqrt(std::max(0.0f, 1.0f - (nxRG * nxRG) - (nyRGFlip * nyRGFlip)));

		const float nxRGB = (r * 2.0f) - 1.0f;
		const float nyRGB = (g * 2.0f) - 1.0f;
		const float nzRGB = (b * 2.0f) - 1.0f;
		const float rgbLength = std::sqrt((nxRGB * nxRGB) + (nyRGB * nyRGB) + (nzRGB * nzRGB));
		const float invRgbLength = (rgbLength > 0.0f ? (1.0f / rgbLength) : 0.0f);

		context->log(
			Context::Info,
			"- sample ({}, {}): RGBA=({:.6f}, {:.6f}, {:.6f}, {:.6f}) RG->N=({:.6f}, {:.6f}, {:.6f}) RG->NflipY=({:.6f}, {:.6f}, {:.6f}) RGB->Nnorm=({:.6f}, {:.6f}, {:.6f})",
			x,
			y,
			r,
			g,
			b,
			a,
			nxRG,
			nyRG,
			nzRG,
			nxRG,
			nyRGFlip,
			nzRGFlip,
			nxRGB * invRgbLength,
			nyRGB * invRgbLength,
			nzRGB * invRgbLength);
	};

	logSample(0, 0);
	logSample(width / 2, height / 2);
	logSample(width - 1, height - 1);
	return true;
}

CompressStatus compressTexture(
	Context *context,
	FileSystem::Path const &inputFilePath,
	CompressionSettings const &settings
#if defined(_WIN32)
	, ID3D11Device *compressionDevice
#endif
)
{
	auto compressStartTime = std::chrono::steady_clock::now();

	if (!inputFilePath.isFile())
	{
		context->log(Context::Error, "Input file not found: {}", inputFilePath.getString());
		return CompressStatus::Failed;
	}

	auto outputFilePath(inputFilePath.withExtension(".ktx2"));
	if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath))
	{
		context->log(Context::Error, "Input file hasn't changed since last compression: {}", inputFilePath.getString());
		return CompressStatus::Skipped;
	}

	std::string extension(String::GetLower(inputFilePath.getExtension()));
	if (extension == ".ktx2")
	{
		context->log(Context::Error, "Input file is already compressed: {}", inputFilePath.getString());
		return CompressStatus::Skipped;
	}

	std::vector<uint8_t> buffer(FileSystem::Load(inputFilePath));
	if (buffer.empty())
	{
		context->log(Context::Error, "Unable to read input file bytes: {}", inputFilePath.getString());
		return CompressStatus::Failed;
	}

	// TODO: Replace with actual image decoding (e.g., stb_image or similar) and conversion to RGBA8/BCn as needed.
	// For demonstration, assume buffer is raw RGBA8 and write as KTX2.
	ktxTexture2* kTexture = nullptr;
	KTX_error_code ktxResult = ktxTexture2_Create(
		KTX_TEXTURE_CREATE_ALLOC_STORAGE,
		KTX_VK_FORMAT_R8G8B8A8_UNORM,
		buffer.size(),
		1, // width (replace with actual)
		1, // height (replace with actual)
		1, // depth
		1, // layers
		1, // faces
		1, // mip levels
		&kTexture);
	if (ktxResult != KTX_SUCCESS || !kTexture)
	{
		context->log(Context::Error, "Unable to create KTX2 texture for: {}", inputFilePath.getString());
		return CompressStatus::Failed;
	}
	// Copy buffer to kTexture->pData (replace with actual image data)
	memcpy(kTexture->pData, buffer.data(), buffer.size());

	auto flags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(0);
	if (settings.enableParallel)
	{
		flags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(flags | ::DirectX::TEX_COMPRESS_PARALLEL);
	}
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
		// Normal maps in this content set rely on authored full RGB tangent-space data
		// (legacy behavior required for mirrored UV surfaces). Keep DDS compressed, but
		// preserve XYZ by using BC7 UNORM instead of BC5 RG reconstruction.
		outputFormat = DXGI_FORMAT_BC7_UNORM;
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

	if (outputFormat == DXGI_FORMAT_BC7_UNORM_SRGB || outputFormat == DXGI_FORMAT_BC7_UNORM)
	{
		flags = static_cast<::DirectX::TEX_COMPRESS_FLAGS>(flags | ::DirectX::TEX_COMPRESS_BC7_QUICK);
	}

	context->log(Context::Info, "Compressing: {} to {}", inputFilePath.getString(), outputFilePath.getString());
    switch (outputFormat)
    {
    case DXGI_FORMAT_BC7_UNORM_SRGB:
        context->log(Context::Info, "- Albedo BC7");
        break;

	case DXGI_FORMAT_BC7_UNORM:
		context->log(Context::Info, "- Normal BC7 (RGB)");
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
		HRESULT compressionResult = E_FAIL;

#if defined(_WIN32)
		const bool allowGPUPath =
			settings.enableGPU &&
			compressionDevice &&
			(outputFormat == DXGI_FORMAT_BC7_UNORM || outputFormat == DXGI_FORMAT_BC7_UNORM_SRGB);

		if (allowGPUPath)
		{
			compressionResult = ::DirectX::CompressEx(
				compressionDevice,
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
							"- {} (GPU) compression in progress: '{}' ({:.1f}% | {} s elapsed)",
							passName,
							inputFilePath.getString(),
							(ratio * 100.0),
							elapsedSeconds);
					}

					return true;
				});

			if (FAILED(compressionResult))
			{
				context->log(
					Context::Warning,
					"- {} GPU compression failed (HRESULT=0x{:08X}); retrying on CPU",
					passName,
					static_cast<uint32_t>(compressionResult));
			}
		}

		if (FAILED(compressionResult))
#endif
		{
			compressionResult = ::DirectX::CompressEx(
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
		}

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

	ktxResult = ktxTexture2_WriteToNamedFile(kTexture, outputFilePath.getString().c_str());
	ktxTexture_Destroy(ktxTexture(kTexture));
	if (ktxResult != KTX_SUCCESS)
	{
		context->log(Context::Info, "Unable to save KTX2 image");
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

		CompressionSettings settings;
		if (auto parallelEnv = std::getenv("GEK_COMPRESS_PARALLEL"))
		{
			settings.enableParallel = IsTruthyEnv(parallelEnv);
		}
		if (auto gpuEnv = std::getenv("GEK_COMPRESS_GPU"))
		{
			settings.enableGPU = IsTruthyEnv(gpuEnv);
		}

		context->log(Context::Info, "Compression configuration: parallel={} gpu={}", settings.enableParallel, settings.enableGPU);

		#if defined(_WIN32)
		CompressionDevice compressionDevice;
		InitializeCompressionDevice(context.get(), settings, compressionDevice);
		#else
		(void)settings;
		#endif

		if (argumentCount > 1)
		{
			bool allSucceeded = true;
			for (int argumentIndex = 1; argumentIndex < argumentCount; ++argumentIndex)
			{
				if (!inspectTexture(context.get(), FileSystem::Path(argumentList[argumentIndex])))
				{
					allSucceeded = false;
				}
			}

			if (SUCCEEDED(comInitializeResult))
			{
				CoUninitialize();
			}

			return allSucceeded ? 0 : 1;
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

				auto status = compressTexture(
					context.get(),
					texturePath,
					settings
					#if defined(_WIN32)
					, compressionDevice.device
					#endif
				);
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