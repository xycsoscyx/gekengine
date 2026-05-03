#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

#include <ktx.h>
#include <bc7enc.h>
#include <rgbcx.h>

// VkFormat values used for KTX2 container (from vulkan/vulkan_core.h)
#define GEK_VK_FORMAT_UNDEFINED          0
#define GEK_VK_FORMAT_BC1_RGB_SRGB_BLOCK 132
#define GEK_VK_FORMAT_BC4_UNORM_BLOCK    139
#define GEK_VK_FORMAT_BC7_UNORM_BLOCK    145
#define GEK_VK_FORMAT_BC7_SRGB_BLOCK     146

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace Gek;

enum class CompressStatus
{
    Skipped,
    Succeeded,
    Failed,
};

enum class CompressionQuality
{
    Fast,
    Medium,
    High,
};

struct CompressionSettings
{
    bool enableParallel = true;
    CompressionQuality quality = CompressionQuality::Medium;
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

CompressionQuality ParseQuality(std::string const &input)
{
    std::string quality = String::GetLower(input);
    if (quality == "fast")
    {
        return CompressionQuality::Fast;
    }

    if (quality == "high")
    {
        return CompressionQuality::High;
    }

    return CompressionQuality::Medium;
}

char const *ToString(CompressionQuality quality)
{
    switch (quality)
    {
    case CompressionQuality::Fast: return "fast";
    case CompressionQuality::High: return "high";
    default: return "medium";
    }
}

enum class TextureClass
{
    Albedo,
    Normal,
    RoughnessLike,
    Unsupported,
};

enum class EncodedFormat
{
    BC1Srgb,
    BC4Unorm,
    BC7Srgb,
    BC7Unorm,
};

TextureClass ClassifyTextureName(std::string const &textureName)
{
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
        return TextureClass::Albedo;
    }

    if (String::EndsWith(textureName, "normal") ||
        String::EndsWith(textureName, "normalmap") ||
        String::EndsWith(textureName, "normalmap_s") ||
        String::EndsWith(textureName, "_ddn") ||
        String::EndsWith(textureName, "_nrm") ||
        String::EndsWith(textureName, "_n"))
    {
        return TextureClass::Normal;
    }

    if (String::EndsWith(textureName, "roughness") ||
        String::EndsWith(textureName, "roughness_s") ||
        String::EndsWith(textureName, "rough") ||
        String::EndsWith(textureName, "_r") ||
        String::EndsWith(textureName, "metalness") ||
        String::EndsWith(textureName, "metallic") ||
        String::EndsWith(textureName, "metal") ||
        String::EndsWith(textureName, "_m"))
    {
        return TextureClass::RoughnessLike;
    }

    return TextureClass::Unsupported;
}

bool HasAnyAlpha(std::vector<uint8_t> const &rgba)
{
    for (size_t pixel = 0; pixel < (rgba.size() / 4); ++pixel)
    {
        if (rgba[(pixel * 4) + 3] < 255)
        {
            return true;
        }
    }

    return false;
}

void GatherBlockRgba(
    std::vector<uint8_t> const &rgba,
    uint32_t width,
    uint32_t height,
    uint32_t blockX,
    uint32_t blockY,
    uint8_t *blockPixels)
{
    for (uint32_t py = 0; py < 4; ++py)
    {
        for (uint32_t px = 0; px < 4; ++px)
        {
            uint32_t sourceX = std::min((blockX * 4) + px, width - 1);
            uint32_t sourceY = std::min((blockY * 4) + py, height - 1);
            size_t sourceIndex = (static_cast<size_t>(sourceY) * width + sourceX) * 4;
            size_t blockIndex = ((py * 4) + px) * 4;
            std::memcpy(blockPixels + blockIndex, rgba.data() + sourceIndex, 4);
        }
    }
}

void CompressToBc1(std::vector<uint8_t> const &rgba, uint32_t width, uint32_t height, CompressionQuality quality, std::vector<uint8_t> &output)
{
    rgbcx::init();

    uint32_t blockWidth = (width + 3) / 4;
    uint32_t blockHeight = (height + 3) / 4;
    output.resize(static_cast<size_t>(blockWidth) * blockHeight * 8);

    uint32_t level = 8;
    if (quality == CompressionQuality::Fast)
    {
        level = 2;
    }
    else if (quality == CompressionQuality::High)
    {
        level = 16;
    }

    std::array<uint8_t, 64> blockPixels{};
    for (uint32_t by = 0; by < blockHeight; ++by)
    {
        for (uint32_t bx = 0; bx < blockWidth; ++bx)
        {
            GatherBlockRgba(rgba, width, height, bx, by, blockPixels.data());
            size_t blockOffset = (static_cast<size_t>(by) * blockWidth + bx) * 8;
            rgbcx::encode_bc1(level, output.data() + blockOffset, blockPixels.data(), false, false);
        }
    }
}

void CompressToBc4(std::vector<uint8_t> const &rgba, uint32_t width, uint32_t height, CompressionQuality quality, std::vector<uint8_t> &output)
{
    rgbcx::init();

    uint32_t blockWidth = (width + 3) / 4;
    uint32_t blockHeight = (height + 3) / 4;
    output.resize(static_cast<size_t>(blockWidth) * blockHeight * 8);

    std::array<uint8_t, 64> blockPixels{};
    for (uint32_t by = 0; by < blockHeight; ++by)
    {
        for (uint32_t bx = 0; bx < blockWidth; ++bx)
        {
            GatherBlockRgba(rgba, width, height, bx, by, blockPixels.data());
            size_t blockOffset = (static_cast<size_t>(by) * blockWidth + bx) * 8;
            if (quality == CompressionQuality::High)
            {
                rgbcx::encode_bc4_hq(output.data() + blockOffset, blockPixels.data(), 4);
            }
            else
            {
                rgbcx::encode_bc4(output.data() + blockOffset, blockPixels.data(), 4);
            }
        }
    }
}

void CompressToBc7(std::vector<uint8_t> const &rgba, uint32_t width, uint32_t height, CompressionQuality quality, std::vector<uint8_t> &output)
{
    static bool encoderInitialized = false;
    if (!encoderInitialized)
    {
        bc7enc_compress_block_init();
        encoderInitialized = true;
    }

    bc7enc_compress_block_params params{};
    bc7enc_compress_block_params_init(&params);
    if (quality == CompressionQuality::Fast)
    {
        params.m_uber_level = 0;
        params.m_max_partitions = 8;
    }
    else if (quality == CompressionQuality::High)
    {
        params.m_uber_level = 4;
        params.m_max_partitions = BC7ENC_MAX_PARTITIONS;
    }
    else
    {
        params.m_uber_level = 2;
        params.m_max_partitions = 32;
    }

    uint32_t blockWidth = (width + 3) / 4;
    uint32_t blockHeight = (height + 3) / 4;
    output.resize(static_cast<size_t>(blockWidth) * blockHeight * 16);

    std::array<uint8_t, 64> blockPixels{};
    for (uint32_t by = 0; by < blockHeight; ++by)
    {
        for (uint32_t bx = 0; bx < blockWidth; ++bx)
        {
            GatherBlockRgba(rgba, width, height, bx, by, blockPixels.data());
            size_t blockOffset = (static_cast<size_t>(by) * blockWidth + bx) * 16;
            bc7enc_compress_block(output.data() + blockOffset, blockPixels.data(), &params);
        }
    }
}

uint32_t GetMipLevelCount(uint32_t width, uint32_t height)
{
    uint32_t levels = 1;
    while (width > 1 || height > 1)
    {
        width = std::max<uint32_t>(1, width / 2);
        height = std::max<uint32_t>(1, height / 2);
        ++levels;
    }

    return levels;
}

void GenerateNextMipLevel(
    std::vector<uint8_t> const &sourceRgba,
    uint32_t sourceWidth,
    uint32_t sourceHeight,
    std::vector<uint8_t> &targetRgba,
    uint32_t &targetWidth,
    uint32_t &targetHeight)
{
    targetWidth = std::max<uint32_t>(1, sourceWidth / 2);
    targetHeight = std::max<uint32_t>(1, sourceHeight / 2);
    targetRgba.resize(static_cast<size_t>(targetWidth) * targetHeight * 4);

    for (uint32_t y = 0; y < targetHeight; ++y)
    {
        for (uint32_t x = 0; x < targetWidth; ++x)
        {
            uint32_t sourceX0 = std::min((x * 2) + 0, sourceWidth - 1);
            uint32_t sourceX1 = std::min((x * 2) + 1, sourceWidth - 1);
            uint32_t sourceY0 = std::min((y * 2) + 0, sourceHeight - 1);
            uint32_t sourceY1 = std::min((y * 2) + 1, sourceHeight - 1);

            std::array<size_t, 4> sampleIndex =
            {
                (static_cast<size_t>(sourceY0) * sourceWidth + sourceX0) * 4,
                (static_cast<size_t>(sourceY0) * sourceWidth + sourceX1) * 4,
                (static_cast<size_t>(sourceY1) * sourceWidth + sourceX0) * 4,
                (static_cast<size_t>(sourceY1) * sourceWidth + sourceX1) * 4,
            };

            size_t targetIndex = (static_cast<size_t>(y) * targetWidth + x) * 4;
            for (size_t channel = 0; channel < 4; ++channel)
            {
                uint32_t value =
                    sourceRgba[sampleIndex[0] + channel] +
                    sourceRgba[sampleIndex[1] + channel] +
                    sourceRgba[sampleIndex[2] + channel] +
                    sourceRgba[sampleIndex[3] + channel];
                targetRgba[targetIndex + channel] = static_cast<uint8_t>(value / 4);
            }
        }
    }
}

bool KtxHasCompleteMipChain(FileSystem::Path const &ktxPath)
{
    ktxTexture2 *texture = nullptr;
    KTX_error_code result = ktxTexture2_CreateFromNamedFile(
        ktxPath.getString().c_str(),
        KTX_TEXTURE_CREATE_NO_FLAGS,
        &texture);

    if (result != KTX_SUCCESS || !texture)
    {
        return false;
    }

    uint32_t expectedLevels = GetMipLevelCount(
        std::max<ktx_uint32_t>(1, texture->baseWidth),
        std::max<ktx_uint32_t>(1, texture->baseHeight));
    bool hasMipChain = (texture->numLevels >= expectedLevels);
    ktxTexture_Destroy(ktxTexture(texture));
    return hasMipChain;
}

bool inspectTexture(Context *context, FileSystem::Path const &inputFilePath)
{
    if (!inputFilePath.isFile())
    {
        context->log(Context::Error, "Texture file not found: {}", inputFilePath.getString());
        return false;
    }

    std::vector<uint8_t> sourceData(FileSystem::Load(inputFilePath));
    if (sourceData.empty())
    {
        context->log(Context::Error, "Unable to read texture bytes: {}", inputFilePath.getString());
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc *decodedPixels = stbi_load_from_memory(sourceData.data(), static_cast<int>(sourceData.size()), &width, &height, &channels, 4);
    if (!decodedPixels)
    {
        context->log(Context::Error, "Unable to decode texture with stb_image: {}", inputFilePath.getString());
        return false;
    }

    std::vector<uint8_t> rgba(decodedPixels, decodedPixels + (static_cast<size_t>(width) * height * 4));
    stbi_image_free(decodedPixels);

    float minR = 1.0f;
    float minG = 1.0f;
    float minB = 1.0f;
    float minA = 1.0f;
    float maxR = 0.0f;
    float maxG = 0.0f;
    float maxB = 0.0f;
    float maxA = 0.0f;

    for (size_t i = 0; i < rgba.size(); i += 4)
    {
        float r = rgba[i + 0] / 255.0f;
        float g = rgba[i + 1] / 255.0f;
        float b = rgba[i + 2] / 255.0f;
        float a = rgba[i + 3] / 255.0f;

        minR = std::min(minR, r);
        minG = std::min(minG, g);
        minB = std::min(minB, b);
        minA = std::min(minA, a);
        maxR = std::max(maxR, r);
        maxG = std::max(maxG, g);
        maxB = std::max(maxB, b);
        maxA = std::max(maxA, a);
    }

    context->log(Context::Info, "Inspecting texture: {}", inputFilePath.getString());
    context->log(Context::Info, "- metadata: {}x{} channels={}", width, height, channels);
    context->log(Context::Info, "- channel range: R=[{:.6f}, {:.6f}] G=[{:.6f}, {:.6f}] B=[{:.6f}, {:.6f}] A=[{:.6f}, {:.6f}]", minR, maxR, minG, maxG, minB, maxB, minA, maxA);

    auto logSample = [&](size_t x, size_t y)
    {
        size_t sampleIndex = ((y * static_cast<size_t>(width)) + x) * 4;
        float r = rgba[sampleIndex + 0] / 255.0f;
        float g = rgba[sampleIndex + 1] / 255.0f;
        float b = rgba[sampleIndex + 2] / 255.0f;
        float a = rgba[sampleIndex + 3] / 255.0f;

        context->log(
            Context::Info,
            "- sample ({}, {}): RGBA=({:.6f}, {:.6f}, {:.6f}, {:.6f})",
            x,
            y,
            r,
            g,
            b,
            a);
    };

    logSample(0, 0);
    logSample(static_cast<size_t>(width) / 2, static_cast<size_t>(height) / 2);
    logSample(static_cast<size_t>(width) - 1, static_cast<size_t>(height) - 1);
    return true;
}

CompressStatus compressTexture(
    Context *context,
    FileSystem::Path const &inputFilePath,
    CompressionSettings const &settings)
{
    auto compressStartTime = std::chrono::steady_clock::now();

    if (!inputFilePath.isFile())
    {
        context->log(Context::Error, "Input file not found: {}", inputFilePath.getString());
        return CompressStatus::Failed;
    }

    auto outputFilePath(inputFilePath.withExtension(".ktx2"));
    if (outputFilePath.isFile() && outputFilePath.isNewerThan(inputFilePath) && KtxHasCompleteMipChain(outputFilePath))
    {
        context->log(Context::Info, "Input file hasn't changed since last compression: {}", inputFilePath.getString());
        return CompressStatus::Skipped;
    }

    std::string extension(String::GetLower(inputFilePath.getExtension()));
    if (extension == ".ktx2" || extension == ".dds")
    {
        context->log(Context::Info, "Skipping unsupported input extension for migration pass: {}", inputFilePath.getString());
        return CompressStatus::Skipped;
    }

    std::vector<uint8_t> sourceData(FileSystem::Load(inputFilePath));
    if (sourceData.empty())
    {
        context->log(Context::Error, "Unable to read input file bytes: {}", inputFilePath.getString());
        return CompressStatus::Failed;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc *decodedPixels = stbi_load_from_memory(sourceData.data(), static_cast<int>(sourceData.size()), &width, &height, &channels, 4);
    if (!decodedPixels)
    {
        context->log(Context::Error, "Unable to decode image: {}", inputFilePath.getString());
        return CompressStatus::Failed;
    }

    std::vector<uint8_t> rgba(decodedPixels, decodedPixels + (static_cast<size_t>(width) * height * 4));
    stbi_image_free(decodedPixels);

    std::string textureName(String::GetLower(inputFilePath.withoutExtension().getString()));
    TextureClass textureClass = ClassifyTextureName(textureName);
    if (textureClass == TextureClass::Unsupported)
    {
        context->log(Context::Info, "Skipping unhandled texture material type: {}", textureName);
        return CompressStatus::Skipped;
    }

    EncodedFormat encodedFormat = EncodedFormat::BC7Unorm;
    if (textureClass == TextureClass::Albedo)
    {
        encodedFormat = HasAnyAlpha(rgba) ? EncodedFormat::BC7Srgb : EncodedFormat::BC1Srgb;
    }
    else if (textureClass == TextureClass::Normal)
    {
        encodedFormat = EncodedFormat::BC7Unorm;
    }
    else if (textureClass == TextureClass::RoughnessLike)
    {
        encodedFormat = EncodedFormat::BC4Unorm;
    }

    ktx_uint32_t vkFormat = GEK_VK_FORMAT_UNDEFINED;

    switch (encodedFormat)
    {
    case EncodedFormat::BC1Srgb:
        vkFormat = GEK_VK_FORMAT_BC1_RGB_SRGB_BLOCK;
        context->log(Context::Info, "- Albedo BC1 sRGB");
        break;

    case EncodedFormat::BC4Unorm:
        vkFormat = GEK_VK_FORMAT_BC4_UNORM_BLOCK;
        context->log(Context::Info, "- Metalness/Roughness BC4");
        break;

    case EncodedFormat::BC7Srgb:
        vkFormat = GEK_VK_FORMAT_BC7_SRGB_BLOCK;
        context->log(Context::Info, "- Albedo BC7 sRGB");
        break;

    case EncodedFormat::BC7Unorm:
        vkFormat = GEK_VK_FORMAT_BC7_UNORM_BLOCK;
        context->log(Context::Info, "- Normal BC7 UNORM");
        break;
    }

    uint32_t mipLevels = GetMipLevelCount(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    context->log(Context::Info, "- Generating {} mip levels", mipLevels);

    ktxTextureCreateInfo createInfo{};
    createInfo.vkFormat = vkFormat;
    createInfo.baseWidth = static_cast<ktx_uint32_t>(width);
    createInfo.baseHeight = static_cast<ktx_uint32_t>(height);
    createInfo.baseDepth = 1;
    createInfo.numDimensions = 2;
    createInfo.numLevels = mipLevels;
    createInfo.numLayers = 1;
    createInfo.numFaces = 1;
    createInfo.isArray = KTX_FALSE;
    createInfo.generateMipmaps = KTX_FALSE;

    ktxTexture2 *kTexture = nullptr;
    KTX_error_code ktxResult = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);
    if (ktxResult != KTX_SUCCESS || !kTexture)
    {
        context->log(Context::Error, "Unable to create KTX2 texture for: {}", inputFilePath.getString());
        return CompressStatus::Failed;
    }

    std::vector<uint8_t> mipRgba = rgba;
    uint32_t mipWidth = static_cast<uint32_t>(width);
    uint32_t mipHeight = static_cast<uint32_t>(height);

    for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
    {
        std::vector<uint8_t> compressedData;
        switch (encodedFormat)
        {
        case EncodedFormat::BC1Srgb:
            CompressToBc1(mipRgba, mipWidth, mipHeight, settings.quality, compressedData);
            break;

        case EncodedFormat::BC4Unorm:
            CompressToBc4(mipRgba, mipWidth, mipHeight, settings.quality, compressedData);
            break;

        case EncodedFormat::BC7Srgb:
        case EncodedFormat::BC7Unorm:
            CompressToBc7(mipRgba, mipWidth, mipHeight, settings.quality, compressedData);
            break;
        }

        ktxResult = ktxTexture_SetImageFromMemory(
            ktxTexture(kTexture),
            mipLevel,
            0,
            0,
            compressedData.data(),
            compressedData.size());
        if (ktxResult != KTX_SUCCESS)
        {
            context->log(Context::Error, "Unable to write compressed mip level {} for: {}", mipLevel, inputFilePath.getString());
            ktxTexture_Destroy(ktxTexture(kTexture));
            return CompressStatus::Failed;
        }

        if ((mipLevel + 1) < mipLevels)
        {
            std::vector<uint8_t> nextMipRgba;
            uint32_t nextMipWidth = 1;
            uint32_t nextMipHeight = 1;
            GenerateNextMipLevel(mipRgba, mipWidth, mipHeight, nextMipRgba, nextMipWidth, nextMipHeight);
            mipRgba = std::move(nextMipRgba);
            mipWidth = nextMipWidth;
            mipHeight = nextMipHeight;
        }
    }

    ktxResult = ktxTexture_WriteToNamedFile(ktxTexture(kTexture), outputFilePath.getString().c_str());
    ktxTexture_Destroy(ktxTexture(kTexture));
    if (ktxResult != KTX_SUCCESS)
    {
        context->log(Context::Error, "Unable to save KTX2 image: {}", outputFilePath.getString());
        return CompressStatus::Failed;
    }

    auto elapsedMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - compressStartTime).count();
    context->log(
        Context::Info,
        "Compressed '{}' to '{}' in {} ms (quality={})",
        inputFilePath.getString(),
        outputFilePath.getString(),
        elapsedMilliseconds,
        ToString(settings.quality));
    return CompressStatus::Succeeded;
}

int main(int argumentCount, char const *const argumentList[])
{
    auto pluginPath(FileSystem::GetModuleFilePath().getParentPath());
    auto engineRootPath(pluginPath.getParentPath());
    auto cachePath(FileSystem::GetCacheFromModule());
    auto rootPath(cachePath.getParentPath());
    cachePath.setWorkingDirectory();

    std::vector<FileSystem::Path> searchPathList;
    searchPathList.push_back(pluginPath);

    ContextPtr context(Context::Create(&searchPathList));
    if (!context)
    {
        return 1;
    }

    context->log(Context::Info, "GEK Texture Compressor");
    context->setCachePath(cachePath);

    context->addDataPath(engineRootPath / "data");
    context->addDataPath(rootPath / "data");
    context->addDataPath(rootPath.getString());

    auto gekDataPath = std::getenv("gek_data_path");
    if (gekDataPath)
    {
        context->addDataPath(gekDataPath);
    }

    CompressionSettings settings;
    if (auto parallelEnv = std::getenv("GEK_COMPRESS_PARALLEL"))
    {
        settings.enableParallel = IsTruthyEnv(parallelEnv);
    }

    if (auto qualityEnv = std::getenv("GEK_COMPRESS_QUALITY"))
    {
        settings.quality = ParseQuality(qualityEnv);
    }

    context->log(
        Context::Info,
        "Compression configuration: parallel={} quality={}",
        settings.enableParallel,
        ToString(settings.quality));

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
            if (!filePath.isFile())
            {
                return true;
            }

            std::string extension = String::GetLower(filePath.getExtension());
            if (extension == ".dds" || extension == ".ktx2")
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

    std::sort(
        std::begin(textureFileList),
        std::end(textureFileList),
        [](FileSystem::Path const &left, FileSystem::Path const &right) -> bool
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

        auto status = compressTexture(context.get(), texturePath, settings);
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
        }

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

    return (failedCount == 0 ? 0 : 1);
}
