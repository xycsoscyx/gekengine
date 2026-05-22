#include "API/System/RenderDevice.hpp"
#include "API/System/WindowDevice.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include <algorithm>

#include <array>
#include <atomic>
#include <bit>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <execution>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <dxgiformat.h>
#include <objbase.h>
#elif defined(__linux__)
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <wayland-client.h>
#endif

// clang-format off
// ktxvulkan.h requires vulkan/vulkan.h to be included first.
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <ktx.h>
#include <ktxvulkan.h>
// clang-format on
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <slang-com-ptr.h>
#include <slang.h>

namespace Gek
{
    namespace Render::Implementation
    {
        static std::atomic_bool gVulkanDeviceShuttingDown{ false };

        static void waitForResourceDestroyIdle(VkDevice device)
        {
            if ((device != VK_NULL_HANDLE) && !gVulkanDeviceShuttingDown.load(std::memory_order_relaxed))
            {
                vkDeviceWaitIdle(device);
            }
        }

        Render::Format GetFormat(VkFormat format)
        {
            switch (format)
            {
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return Render::Format::R32G32B32A32_FLOAT;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return Render::Format::R16G16B16A16_FLOAT;
            case VK_FORMAT_R32G32B32_SFLOAT:
                return Render::Format::R32G32B32_FLOAT;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return Render::Format::R11G11B10_FLOAT;
            case VK_FORMAT_R32G32_SFLOAT:
                return Render::Format::R32G32_FLOAT;
            case VK_FORMAT_R16G16_SFLOAT:
                return Render::Format::R16G16_FLOAT;
            case VK_FORMAT_R32_SFLOAT:
                return Render::Format::R32_FLOAT;
            case VK_FORMAT_R16_SFLOAT:
                return Render::Format::R16_FLOAT;

            case VK_FORMAT_R32G32B32A32_UINT:
                return Render::Format::R32G32B32A32_UINT;
            case VK_FORMAT_R16G16B16A16_UINT:
                return Render::Format::R16G16B16A16_UINT;
            // case VK_FORMAT_R10G10B10A2_UINT: return Render::Format::R10G10B10A2_UINT;
            case VK_FORMAT_R8G8B8A8_UINT:
                return Render::Format::R8G8B8A8_UINT;
            case VK_FORMAT_R32G32B32_UINT:
                return Render::Format::R32G32B32_UINT;
            case VK_FORMAT_R32G32_UINT:
                return Render::Format::R32G32_UINT;
            case VK_FORMAT_R16G16_UINT:
                return Render::Format::R16G16_UINT;
            case VK_FORMAT_R8G8_UINT:
                return Render::Format::R8G8_UINT;
            case VK_FORMAT_R32_UINT:
                return Render::Format::R32_UINT;
            case VK_FORMAT_R16_UINT:
                return Render::Format::R16_UINT;
            case VK_FORMAT_R8_UINT:
                return Render::Format::R8_UINT;

            case VK_FORMAT_R32G32B32A32_SINT:
                return Render::Format::R32G32B32A32_INT;
            case VK_FORMAT_R16G16B16A16_SINT:
                return Render::Format::R16G16B16A16_INT;
            case VK_FORMAT_R8G8B8A8_SINT:
                return Render::Format::R8G8B8A8_INT;
            case VK_FORMAT_R32G32B32_SINT:
                return Render::Format::R32G32B32_INT;
            case VK_FORMAT_R32G32_SINT:
                return Render::Format::R32G32_INT;
            case VK_FORMAT_R16G16_SINT:
                return Render::Format::R16G16_INT;
            case VK_FORMAT_R8G8_SINT:
                return Render::Format::R8G8_INT;
            case VK_FORMAT_R32_SINT:
                return Render::Format::R32_INT;
            case VK_FORMAT_R16_SINT:
                return Render::Format::R16_INT;
            case VK_FORMAT_R8_SINT:
                return Render::Format::R8_INT;

            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return Render::Format::D32_FLOAT_S8X24_UINT;
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return Render::Format::D24_UNORM_S8_UINT;
            case VK_FORMAT_D32_SFLOAT:
                return Render::Format::D32_FLOAT;
            case VK_FORMAT_D16_UNORM:
                return Render::Format::D16_UNORM;

            case VK_FORMAT_R16G16B16A16_UNORM:
                return Render::Format::R16G16B16A16_UNORM;
            // case VK_FORMAT_R10G10B10A2_UNORM: return Render::Format::R10G10B10A2_UNORM;
            case VK_FORMAT_R8G8B8A8_UNORM:
                return Render::Format::R8G8B8A8_UNORM;
            case VK_FORMAT_R8G8B8A8_SRGB:
                return Render::Format::R8G8B8A8_UNORM_SRGB;
            case VK_FORMAT_R16G16_UNORM:
                return Render::Format::R16G16_UNORM;
            case VK_FORMAT_R8G8_UNORM:
                return Render::Format::R8G8_UNORM;
            case VK_FORMAT_R16_UNORM:
                return Render::Format::R16_UNORM;
            case VK_FORMAT_R8_UNORM:
                return Render::Format::R8_UNORM;

            case VK_FORMAT_R16G16B16A16_SNORM:
                return Render::Format::R16G16B16A16_NORM;
            case VK_FORMAT_R8G8B8A8_SNORM:
                return Render::Format::R8G8B8A8_NORM;
            case VK_FORMAT_R16G16_SNORM:
                return Render::Format::R16G16_NORM;
            case VK_FORMAT_R8G8_SNORM:
                return Render::Format::R8G8_NORM;
            case VK_FORMAT_R16_SNORM:
                return Render::Format::R16_NORM;
            case VK_FORMAT_R8_SNORM:
                return Render::Format::R8_NORM;
            };

            return Render::Format::Unknown;
        }

        VkFormat GetVkFormat(Render::Format format)
        {
            switch (format)
            {
            case Render::Format::R32G32B32A32_FLOAT:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Render::Format::R16G16B16A16_FLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Render::Format::R32G32B32_FLOAT:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case Render::Format::R32G32_FLOAT:
                return VK_FORMAT_R32G32_SFLOAT;
            case Render::Format::R16G16_FLOAT:
                return VK_FORMAT_R16G16_SFLOAT;
            case Render::Format::R32_FLOAT:
                return VK_FORMAT_R32_SFLOAT;
            case Render::Format::R16_FLOAT:
                return VK_FORMAT_R16_SFLOAT;
            case Render::Format::R11G11B10_FLOAT:
                return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case Render::Format::R32G32B32A32_UINT:
                return VK_FORMAT_R32G32B32A32_UINT;
            case Render::Format::R16G16B16A16_UINT:
                return VK_FORMAT_R16G16B16A16_UINT;
            case Render::Format::R8G8B8A8_UINT:
                return VK_FORMAT_R8G8B8A8_UINT;
            case Render::Format::R32G32B32_UINT:
                return VK_FORMAT_R32G32B32_UINT;
            case Render::Format::R32G32_UINT:
                return VK_FORMAT_R32G32_UINT;
            case Render::Format::R16G16_UINT:
                return VK_FORMAT_R16G16_UINT;
            case Render::Format::R8G8_UINT:
                return VK_FORMAT_R8G8_UINT;
            case Render::Format::R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case Render::Format::R8G8B8A8_UNORM_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;

            case Render::Format::R32G32B32A32_INT:
                return VK_FORMAT_R32G32B32A32_SINT;
            case Render::Format::R16G16B16A16_INT:
                return VK_FORMAT_R16G16B16A16_SINT;
            case Render::Format::R8G8B8A8_INT:
                return VK_FORMAT_R8G8B8A8_SINT;
            case Render::Format::R32G32B32_INT:
                return VK_FORMAT_R32G32B32_SINT;
            case Render::Format::R32G32_INT:
                return VK_FORMAT_R32G32_SINT;
            case Render::Format::R16G16_INT:
                return VK_FORMAT_R16G16_SINT;
            case Render::Format::R8G8_INT:
                return VK_FORMAT_R8G8_SINT;

            case Render::Format::R16G16_UNORM:
                return VK_FORMAT_R16G16_UNORM;
            case Render::Format::R8G8_UNORM:
                return VK_FORMAT_R8G8_UNORM;
            case Render::Format::R16_UNORM:
                return VK_FORMAT_R16_UNORM;
            case Render::Format::R8_UNORM:
                return VK_FORMAT_R8_UNORM;

            case Render::Format::R16G16_NORM:
                return VK_FORMAT_R16G16_SNORM;
            case Render::Format::R8G8_NORM:
                return VK_FORMAT_R8G8_SNORM;
            case Render::Format::R16_NORM:
                return VK_FORMAT_R16_SNORM;
            case Render::Format::R8_NORM:
                return VK_FORMAT_R8_SNORM;

            case Render::Format::R16_UINT:
                return VK_FORMAT_R16_UINT;
            case Render::Format::R32_UINT:
                return VK_FORMAT_R32_UINT;
            case Render::Format::D32_FLOAT_S8X24_UINT:
                return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case Render::Format::D24_UNORM_S8_UINT:
                return VK_FORMAT_D24_UNORM_S8_UINT;
            case Render::Format::D32_FLOAT:
                return VK_FORMAT_D32_SFLOAT;
            case Render::Format::D16_UNORM:
                return VK_FORMAT_D16_UNORM;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }

#ifdef _WIN32
        DXGI_FORMAT ResolveDxgiFormatForSrgbPreference(DXGI_FORMAT format, bool preferSrgb)
        {
            if (preferSrgb)
            {
                switch (format)
                {
                case DXGI_FORMAT_BC1_UNORM:
                    return DXGI_FORMAT_BC1_UNORM_SRGB;
                case DXGI_FORMAT_BC2_UNORM:
                    return DXGI_FORMAT_BC2_UNORM_SRGB;
                case DXGI_FORMAT_BC3_UNORM:
                    return DXGI_FORMAT_BC3_UNORM_SRGB;
                case DXGI_FORMAT_BC7_UNORM:
                    return DXGI_FORMAT_BC7_UNORM_SRGB;
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                default:
                    break;
                }
            }
            else
            {
                switch (format)
                {
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                    return DXGI_FORMAT_BC1_UNORM;
                case DXGI_FORMAT_BC2_UNORM_SRGB:
                    return DXGI_FORMAT_BC2_UNORM;
                case DXGI_FORMAT_BC3_UNORM_SRGB:
                    return DXGI_FORMAT_BC3_UNORM;
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                    return DXGI_FORMAT_BC7_UNORM;
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                default:
                    break;
                }
            }

            return format;
        }

        VkFormat ConvertDxgiToVkFormat(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case DXGI_FORMAT_BC1_UNORM:
                return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case DXGI_FORMAT_BC2_UNORM:
                return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                return VK_FORMAT_BC2_SRGB_BLOCK;
            case DXGI_FORMAT_BC3_UNORM:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return VK_FORMAT_BC3_SRGB_BLOCK;
            case DXGI_FORMAT_BC4_UNORM:
                return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_SNORM:
                return VK_FORMAT_BC4_SNORM_BLOCK;
            case DXGI_FORMAT_BC5_UNORM:
                return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_SNORM:
                return VK_FORMAT_BC5_SNORM_BLOCK;
            case DXGI_FORMAT_BC7_UNORM:
                return VK_FORMAT_BC7_UNORM_BLOCK;
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return VK_FORMAT_BC7_SRGB_BLOCK;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }

        std::pair<size_t, size_t> GetBlockCount(DXGI_FORMAT format, uint32_t width, uint32_t height)
        {
            switch (format)
            {
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return { std::max<size_t>(1, (width + 3u) / 4u), std::max<size_t>(1, (height + 3u) / 4u) };
            default:
                return { static_cast<size_t>(std::max(width, 1u)), static_cast<size_t>(std::max(height, 1u)) };
            }
        }

        size_t GetRowPitch(DXGI_FORMAT format, size_t blocksWide)
        {
            switch (format)
            {
            case DXGI_FORMAT_BC1_UNORM:
            case DXGI_FORMAT_BC1_UNORM_SRGB:
            case DXGI_FORMAT_BC4_UNORM:
            case DXGI_FORMAT_BC4_SNORM:
                return blocksWide * 8;
            case DXGI_FORMAT_BC2_UNORM:
            case DXGI_FORMAT_BC2_UNORM_SRGB:
            case DXGI_FORMAT_BC3_UNORM:
            case DXGI_FORMAT_BC3_UNORM_SRGB:
            case DXGI_FORMAT_BC5_UNORM:
            case DXGI_FORMAT_BC5_SNORM:
            case DXGI_FORMAT_BC7_UNORM:
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                return blocksWide * 16;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return blocksWide * 4;
            default:
                return 0;
            }
        }

        size_t GetSlicePitch(DXGI_FORMAT format, size_t blocksWide, size_t blocksHigh)
        {
            const size_t rowPitch = GetRowPitch(format, blocksWide);
            if (rowPitch == 0)
            {
                return 0;
            }

            return rowPitch * blocksHigh;
        }
#endif

        uint32_t GetFormatStride(Render::Format format)
        {
            switch (format)
            {
            case Render::Format::R32G32B32A32_FLOAT:
                return 16;
            case Render::Format::R16G16B16A16_FLOAT:
                return 8;
            case Render::Format::R32G32B32_FLOAT:
                return 12;
            case Render::Format::R32G32_FLOAT:
                return 8;
            case Render::Format::R16G16_FLOAT:
                return 4;
            case Render::Format::R32_FLOAT:
                return 4;
            case Render::Format::R16_FLOAT:
                return 2;

            case Render::Format::R32G32B32A32_UINT:
                return 16;
            case Render::Format::R16G16B16A16_UINT:
                return 8;
            case Render::Format::R8G8B8A8_UINT:
                return 4;
            case Render::Format::R32G32B32_UINT:
                return 12;
            case Render::Format::R32G32_UINT:
                return 8;
            case Render::Format::R16G16_UINT:
                return 4;
            case Render::Format::R8G8_UINT:
                return 2;

            case Render::Format::R32G32B32A32_INT:
                return 16;
            case Render::Format::R16G16B16A16_INT:
                return 8;
            case Render::Format::R8G8B8A8_INT:
                return 4;
            case Render::Format::R32G32B32_INT:
                return 12;
            case Render::Format::R32G32_INT:
                return 8;
            case Render::Format::R16G16_INT:
                return 4;
            case Render::Format::R8G8_INT:
                return 2;

            case Render::Format::R8G8B8A8_UNORM:
                return 4;
            case Render::Format::R8G8B8A8_UNORM_SRGB:
                return 4;
            case Render::Format::R16G16_UNORM:
                return 4;
            case Render::Format::R8G8_UNORM:
                return 2;
            case Render::Format::R16_UNORM:
                return 2;
            case Render::Format::R8_UNORM:
                return 1;

            case Render::Format::R16G16_NORM:
                return 4;
            case Render::Format::R8G8_NORM:
                return 2;
            case Render::Format::R16_NORM:
                return 2;
            case Render::Format::R8_NORM:
                return 1;

            case Render::Format::R16_UINT:
                return 2;
            case Render::Format::R32_UINT:
                return 4;
            default:
                return 0;
            }
        }

        VkPrimitiveTopology GetVkPrimitiveTopology(Render::PrimitiveType primitiveType)
        {
            switch (primitiveType)
            {
            case Render::PrimitiveType::PointList:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case Render::PrimitiveType::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case Render::PrimitiveType::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case Render::PrimitiveType::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case Render::PrimitiveType::TriangleList:
            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            }
        }

        const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation",
        };

        const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete()
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

        static constexpr std::string_view SemanticNameList[] = {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMAL",
            "NORMAL",
            "COLOR",
        };

        std::string ToUpperAscii(std::string_view value)
        {
            std::string normalized(value);
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](char character)
                           { return static_cast<char>(std::toupper(static_cast<unsigned char>(character))); });
            return normalized;
        }

        bool SplitSemanticToken(std::string_view token, std::string &semanticName, uint32_t &semanticIndex)
        {
            if (token.empty())
            {
                return false;
            }

            size_t suffixStart = token.size();
            while (suffixStart > 0 && std::isdigit(static_cast<unsigned char>(token[suffixStart - 1])) != 0)
            {
                --suffixStart;
            }

            semanticName = ToUpperAscii(token.substr(0, suffixStart));
            semanticIndex = 0;
            if (suffixStart < token.size())
            {
                semanticIndex = static_cast<uint32_t>(std::strtoul(std::string(token.substr(suffixStart)).c_str(), nullptr, 10));
            }

            return !semanticName.empty();
        }

        void AppendVertexInputSignatures(
            slang::VariableLayoutReflection *variableLayout,
            std::vector<Render::Program::Information::VertexInputSignature> &signatures,
            std::set<uint32_t> &usedLocations)
        {
            if (!variableLayout)
            {
                return;
            }

            const char *semanticToken = variableLayout->getSemanticName();
            if (semanticToken && semanticToken[0] != '\0')
            {
                // Leaf variable with a semantic — record it and do not recurse.
                std::string semanticName;
                uint32_t embeddedSemanticIndex = 0;
                if (SplitSemanticToken(semanticToken, semanticName, embeddedSemanticIndex))
                {
                    if (!semanticName.starts_with("SV_"))
                    {
                        const size_t rawOffset = variableLayout->getOffset(slang::ParameterCategory::VaryingInput);
                        if (rawOffset < SLANG_UNKNOWN_SIZE)
                        {
                            const uint32_t absoluteLocation = static_cast<uint32_t>(rawOffset);
                            const uint32_t semanticIndex = static_cast<uint32_t>(variableLayout->getSemanticIndex()) + embeddedSemanticIndex;
                            if (usedLocations.insert(absoluteLocation).second)
                            {
                                Render::Program::Information::VertexInputSignature signature;
                                signature.semanticName = std::move(semanticName);
                                signature.semanticIndex = semanticIndex;
                                signature.shaderLocation = absoluteLocation;
                                signatures.push_back(std::move(signature));
                            }
                        }
                    }
                }
                return;
            }

            // No semantic on this variable — it is a struct; descend into its fields.
            auto *typeLayout = variableLayout->getTypeLayout();
            if (!typeLayout)
            {
                return;
            }

            for (uint32_t fieldIndex = 0; fieldIndex < typeLayout->getFieldCount(); ++fieldIndex)
            {
                AppendVertexInputSignatures(typeLayout->getFieldByIndex(fieldIndex), signatures, usedLocations);
            }
        }

        void ExtractVertexInputSignaturesFromReflection(
            slang::ProgramLayout *programLayout,
            std::vector<Render::Program::Information::VertexInputSignature> &signatures)
        {
            signatures.clear();

            if (!programLayout || programLayout->getEntryPointCount() == 0)
            {
                return;
            }

            auto *entryPoint = programLayout->getEntryPointByIndex(0);
            if (!entryPoint)
            {
                return;
            }

            std::set<uint32_t> usedLocations;
            for (uint32_t parameterIndex = 0; parameterIndex < entryPoint->getParameterCount(); ++parameterIndex)
            {
                AppendVertexInputSignatures(entryPoint->getParameterByIndex(parameterIndex), signatures, usedLocations);
            }

            std::sort(signatures.begin(), signatures.end(), [](const auto &left, const auto &right)
                      {
                          if (left.shaderLocation != right.shaderLocation)
                          {
                              return left.shaderLocation < right.shaderLocation;
                          }

                          if (left.semanticName != right.semanticName)
                          {
                              return left.semanticName < right.semanticName;
                          }

                          return left.semanticIndex < right.semanticIndex; });
        }

        template <typename CONVERT, typename SOURCE>
        auto getObject(SOURCE *source)
        {
            return dynamic_cast<CONVERT *>(source);
        }

        template <typename TYPE>
        struct ObjectCache
        {
            std::vector<TYPE *> objectList;

            template <typename CONVERT, typename INPUT>
            void set(const std::vector<INPUT> &inputList)
            {
                size_t listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    objectList[object] = getObject<CONVERT>(inputList[object]);
                }
            }

            void clear(size_t listCount)
            {
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    objectList[object] = nullptr;
                }
            }

            TYPE *const *const get(void) const
            {
                return objectList.data();
            }
        };

        template <int UNIQUE>
        class BaseObject
        {
          public:
            BaseObject(void)
            {
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "object";
            }
        };

        template <int UNIQUE, typename BASE = Render::Object>
        class BaseVideoObject
            : public BASE
        {
          public:
          public:
            BaseVideoObject(void)
            {
            }

            virtual ~BaseVideoObject(void)
            {
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "video_object";
            }
        };

        template <typename BASE>
        class DescribedVideoObject
            : public BASE
        {
          public:
            typename BASE::Description description;

          public:
            DescribedVideoObject(typename BASE::Description const &description)
                : description(description)
            {
            }

            virtual ~DescribedVideoObject(void)
            {
            }

            typename BASE::Description const &getDescription(void) const
            {
                return description;
            }


            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }
        };

        class CommandList
            : public BaseVideoObject<1>
        {
          public:
            uint64_t identifier = 0;
        };
        using RenderState = DescribedVideoObject<Render::RenderState>;
        using DepthState = DescribedVideoObject<Render::DepthState>;
        using BlendState = DescribedVideoObject<Render::BlendState>;
        class SamplerState
            : public DescribedVideoObject<Render::SamplerState>
        {
          public:
            VkDevice device = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            SamplerState(VkDevice device, Render::SamplerState::Description const &description)
                : DescribedVideoObject<Render::SamplerState>(description), device(device)
            {
            }

            virtual ~SamplerState(void)
            {
                if (gVulkanDeviceShuttingDown.load(std::memory_order_relaxed))
                {
                    sampler = VK_NULL_HANDLE;
                    device = VK_NULL_HANDLE;
                    return;
                }

                waitForResourceDestroyIdle(device);

                if (sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(device, sampler, nullptr);
                    sampler = VK_NULL_HANDLE;
                }
            }
        };
        class InputLayout
            : public BaseVideoObject<2>
        {
          public:
            std::vector<Render::InputElement> elementList;
            std::vector<uint32_t> shaderLocationList;

            InputLayout(const std::vector<Render::InputElement> &elementList, std::vector<uint32_t> shaderLocationList)
                : elementList(elementList), shaderLocationList(std::move(shaderLocationList))
            {
            }
        };

        using Resource = BaseObject<2>;
        using ShaderResourceView = BaseObject<3>;
        using UnorderedAccessView = BaseObject<4>;
        using RenderTargetView = BaseObject<5>;

        class Query
            : public Render::Query
        {
          public:
          public:
            Query(void)
            {
            }

            virtual ~Query(void)
            {
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "query";
            }
        };

        class Buffer
            : public Render::Buffer,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            static constexpr uint32_t VersionSlotCount = 3;

            Render::Buffer::Description description;
            VkDevice device = VK_NULL_HANDLE;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void *mappedData = nullptr;
            VkDeviceSize size = 0;
            bool usesVersionedConstantBacking = false;
            uint32_t activeVersionIndex = 0;
            std::array<VkBuffer, VersionSlotCount> versionBufferList{};
            std::array<VkDeviceMemory, VersionSlotCount> versionMemoryList{};
            std::array<void *, VersionSlotCount> versionMappedDataList{};
            std::array<bool, VersionSlotCount> versionInUseList{};

          public:
            Buffer(VkDevice device, const Render::Buffer::Description &description)
                : Resource(), ShaderResourceView(), UnorderedAccessView(), description(description), device(device)
            {
            }

            virtual ~Buffer(void)
            {
                if (gVulkanDeviceShuttingDown.load(std::memory_order_relaxed))
                {
                    mappedData = nullptr;
                    buffer = VK_NULL_HANDLE;
                    memory = VK_NULL_HANDLE;
                    device = VK_NULL_HANDLE;
                    return;
                }

                waitForResourceDestroyIdle(device);

                if (usesVersionedConstantBacking)
                {
                    for (uint32_t slot = 0; slot < VersionSlotCount; ++slot)
                    {
                        if (versionMappedDataList[slot])
                        {
                            vkUnmapMemory(device, versionMemoryList[slot]);
                            versionMappedDataList[slot] = nullptr;
                        }

                        if (versionBufferList[slot] != VK_NULL_HANDLE)
                        {
                            vkDestroyBuffer(device, versionBufferList[slot], nullptr);
                            versionBufferList[slot] = VK_NULL_HANDLE;
                        }

                        if (versionMemoryList[slot] != VK_NULL_HANDLE)
                        {
                            vkFreeMemory(device, versionMemoryList[slot], nullptr);
                            versionMemoryList[slot] = VK_NULL_HANDLE;
                        }
                    }

                    mappedData = nullptr;
                    buffer = VK_NULL_HANDLE;
                    memory = VK_NULL_HANDLE;
                    return;
                }

                if (mappedData)
                {
                    vkUnmapMemory(device, memory);
                    mappedData = nullptr;
                }

                if (buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(device, buffer, nullptr);
                    buffer = VK_NULL_HANDLE;
                }

                if (memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, memory, nullptr);
                    memory = VK_NULL_HANDLE;
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Buffer
            const Render::Buffer::Description &getDescription(void) const
            {
                return description;
            }
        };

        class BaseTexture
        {
          public:
            Render::Texture::Description description;

          public:
            BaseTexture(const Render::Texture::Description &description)
                : description(description)
            {
            }
        };

        class Texture
            : virtual public Render::Texture,
              public BaseTexture
        {
          public:
            Texture(const Render::Texture::Description &description)
                : BaseTexture(description)
            {
            }

            virtual ~Texture(void) = default;

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Texture
            const Render::Texture::Description &getDescription(void) const
            {
                return description;
            }
        };

        class ViewTexture
            : public Texture,
              public Resource,
              public ShaderResourceView
        {
          public:
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            ViewTexture(const Render::Texture::Description &description)
                : Texture(description), Resource(), ShaderResourceView()
            {
            }

            virtual ~ViewTexture(void)
            {
                waitForResourceDestroyIdle(device);

                if (sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(device, sampler, nullptr);
                }

                if (imageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                }

                if (image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(device, image, nullptr);
                }

                if (memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, memory, nullptr);
                }
            }
        };

        class UnorderedViewTexture
            : public Texture,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            UnorderedViewTexture(const Render::Texture::Description &description)
                : Texture(description), Resource(), ShaderResourceView(), UnorderedAccessView()
            {
            }
        };

        class Target
            : virtual public Render::Target,
              public BaseTexture
        {
          public:
            Render::ViewPort viewPort;

          public:
            Target(const Render::Texture::Description &description)
                : BaseTexture(description), viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(description.width), float(description.height)), 0.0f, 1.0f)
            {
            }

            virtual ~Target(void) = default;

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Texture
            const Render::Texture::Description &getDescription(void) const
            {
                return description;
            }

            // Render::Target
            const Render::ViewPort &getViewPort(void) const
            {
                return viewPort;
            }
        };

        class TargetTexture
            : public Target,
              public Resource,
              public RenderTargetView
        {
          public:
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;
            VkFormat actualFormat = VK_FORMAT_UNDEFINED;
            VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            bool supportsMipBlit = false;

            TargetTexture(const Render::Texture::Description &description)
                : Target(description), Resource(), RenderTargetView()
            {
            }

            virtual ~TargetTexture(void)
            {
                waitForResourceDestroyIdle(device);

                if (sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(device, sampler, nullptr);
                    sampler = VK_NULL_HANDLE;
                }

                if (imageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                    imageView = VK_NULL_HANDLE;
                }

                if (image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(device, image, nullptr);
                    image = VK_NULL_HANDLE;
                }

                if (memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, memory, nullptr);
                    memory = VK_NULL_HANDLE;
                }
            }
        };

        class TargetViewTexture
            : virtual public TargetTexture,
              public ShaderResourceView
        {
          public:
            TargetViewTexture(const Render::Texture::Description &description)
                : TargetTexture(description), ShaderResourceView()
            {
            }

            virtual ~TargetViewTexture(void) = default;
        };

        class UnorderedTargetViewTexture
            : virtual public TargetTexture,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            UnorderedTargetViewTexture(const Render::Texture::Description &description)
                : TargetTexture(description), ShaderResourceView(), UnorderedAccessView()
            {
            }

            virtual ~UnorderedTargetViewTexture(void) = default;
        };

        class DepthTexture
            : public Texture,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

          public:
            DepthTexture(const Render::Texture::Description &description)
                : Texture(description), Resource(), ShaderResourceView(), UnorderedAccessView()
            {
            }

            virtual ~DepthTexture(void)
            {
                waitForResourceDestroyIdle(device);

                if (sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(device, sampler, nullptr);
                    sampler = VK_NULL_HANDLE;
                }

                if (imageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                    imageView = VK_NULL_HANDLE;
                }

                if (image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(device, image, nullptr);
                    image = VK_NULL_HANDLE;
                }

                if (memory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, memory, nullptr);
                    memory = VK_NULL_HANDLE;
                }
            }
        };

        template <int UNIQUE>
        class Program : public Render::Program
        {
          public:
            Render::Program::Information information;
            VkDevice device = VK_NULL_HANDLE;
            VkShaderModule shaderModule = VK_NULL_HANDLE;

          public:
            Program(VkDevice device, Render::Program::Information information)
                : information(information), device(device)
            {

                if (!information.compiledData.empty())
                {
                    if ((information.compiledData.size() < sizeof(uint32_t)) || ((information.compiledData.size() % sizeof(uint32_t)) != 0))
                    {
                        shaderModule = VK_NULL_HANDLE;
                        return;
                    }

                    VkShaderModuleCreateInfo createInfo{};
                    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    createInfo.codeSize = information.compiledData.size();
                    createInfo.pCode = reinterpret_cast<const uint32_t *>(information.compiledData.data());
                    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
                    {
                        shaderModule = VK_NULL_HANDLE;
                    }
                }
            }

            virtual ~Program(void)
            {
                waitForResourceDestroyIdle(device);

                if (shaderModule != VK_NULL_HANDLE)
                {
                    vkDestroyShaderModule(device, shaderModule, nullptr);
                    shaderModule = VK_NULL_HANDLE;
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return information.name;
            }

            // Render::Program
            Information const &getInformation(void) const
            {
                return information;
            }
        };

        using ComputeProgram = Program<1>;
        using VertexProgram = Program<2>;
        using GeometryProgram = Program<3>;
        using PixelProgram = Program<4>;

        GEK_CONTEXT_USER(Device, Window::Device *, Render::Device::Description)
        , public Render::Debug::Device
        {
            struct DrawCommand;
            class Context;

            void enqueueGenerateMipMapsCommand(Context * sourceContext, Render::Texture * texture);
            void enqueueComputeDispatchCommand(Context * sourceContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
            void enqueueClearRenderTargetCommand(Context * sourceContext, Render::Target * renderTarget, Math::Float4 const &clearColor);
            void enqueueClearDepthStencilCommand(Context * sourceContext, Render::Object * depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil);
            void enqueueCopyResourceCommand(Context * sourceContext, Render::Object * destination, Render::Object * source);
            void frameTransitionSwapChainImage(VkImageLayout newLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask);
            void ensureSampledLayoutForView(VkImageView imageView, VkPipelineStageFlags dstStageMask);
            VkImageLayout getSampledImageLayoutForView(VkImageView imageView) const;
            bool ensureFrameRecording();
            void recordCommand(DrawCommand & drawCommand);
            bool endFrameRecording();

            class Context
                : public Render::Device::Context
            {
                static constexpr uint32_t ContextResourceSlotCount = 16;

                class ComputePipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;

                  public:
                    ComputePipeline(Context *context)
                        : context(context)
                    {
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Compute;
                    }

                    void setProgram(Render::Program *program)
                    {
                        context->currentComputeProgram = getObject<ComputeProgram>(program);
                    }

                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t samplerIndex = 0; samplerIndex < list.size(); ++samplerIndex)
                        {
                            const uint32_t slot = firstStage + samplerIndex;
                            if (slot >= context->currentComputeResourceSamplers.size())
                            {
                                continue;
                            }

                            auto samplerState = getObject<SamplerState>(list[samplerIndex]);
                            context->currentComputeResourceSamplers[slot] = (samplerState ? samplerState->sampler : VK_NULL_HANDLE);
                        }
                    }

                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t bufferIndex = 0; bufferIndex < list.size(); ++bufferIndex)
                        {
                            const uint32_t slot = firstStage + bufferIndex;
                            if (slot >= context->currentComputeConstantBuffers.size())
                            {
                                continue;
                            }

                            context->currentComputeConstantBuffers[slot] = getObject<Buffer>(list[bufferIndex]);
                        }
                    }

                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t resourceIndex = 0; resourceIndex < list.size(); ++resourceIndex)
                        {
                            const uint32_t slot = firstStage + resourceIndex;
                            if (slot >= context->currentComputeResourceImageViews.size())
                            {
                                continue;
                            }

                            auto *resource = list[resourceIndex];
                            VkImageView imageView = VK_NULL_HANDLE;
                            VkSampler imageSampler = VK_NULL_HANDLE;
                            if (auto viewTexture = getObject<ViewTexture>(resource))
                            {
                                imageView = viewTexture->imageView;
                                imageSampler = viewTexture->sampler;
                                context->currentComputeResourceBuffers[slot] = nullptr;
                            }
                            else if (auto targetTexture = getObject<TargetTexture>(resource))
                            {
                                imageView = targetTexture->imageView;
                                imageSampler = targetTexture->sampler;
                                context->currentComputeResourceBuffers[slot] = nullptr;
                            }
                            else if (auto depthTexture = getObject<DepthTexture>(resource))
                            {
                                imageView = depthTexture->imageView;
                                imageSampler = depthTexture->sampler;
                                context->currentComputeResourceBuffers[slot] = nullptr;
                            }
                            else if (auto resourceBuffer = getObject<Buffer>(resource))
                            {
                                context->currentComputeResourceBuffers[slot] = resourceBuffer;
                                imageView = VK_NULL_HANDLE;
                                imageSampler = VK_NULL_HANDLE;
                            }
                            else
                            {
                                context->currentComputeResourceBuffers[slot] = nullptr;
                            }

                            context->currentComputeResources[slot] = resource;
                            context->currentComputeResourceImageViews[slot] = imageView;
                            context->currentComputeResourceSamplers[slot] = imageSampler;
                        }
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t resourceIndex = 0; resourceIndex < list.size(); ++resourceIndex)
                        {
                            const uint32_t slot = firstStage + resourceIndex;
                            if (slot >= context->currentComputeUnorderedAccessImageViews.size())
                            {
                                continue;
                            }

                            auto *resource = list[resourceIndex];
                            context->currentComputeUnorderedAccessResources[slot] = resource;

                            if (auto targetTexture = getObject<TargetTexture>(resource))
                            {
                                context->currentComputeUnorderedAccessImageViews[slot] = targetTexture->imageView;
                                context->currentComputeUnorderedAccessBuffers[slot] = nullptr;
                            }
                            else if (auto depthTexture = getObject<DepthTexture>(resource))
                            {
                                context->currentComputeUnorderedAccessImageViews[slot] = depthTexture->imageView;
                                context->currentComputeUnorderedAccessBuffers[slot] = nullptr;
                            }
                            else if (auto resourceBuffer = getObject<Buffer>(resource))
                            {
                                context->currentComputeUnorderedAccessImageViews[slot] = VK_NULL_HANDLE;
                                context->currentComputeUnorderedAccessBuffers[slot] = resourceBuffer;
                            }
                            else
                            {
                                context->currentComputeUnorderedAccessImageViews[slot] = VK_NULL_HANDLE;
                                context->currentComputeUnorderedAccessBuffers[slot] = nullptr;
                            }
                        }
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentComputeResourceSamplers.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentComputeResourceSamplers.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentComputeResourceSamplers[stage] = VK_NULL_HANDLE;
                        }
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentComputeConstantBuffers.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentComputeConstantBuffers.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentComputeConstantBuffers[stage] = nullptr;
                        }
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentComputeResourceImageViews.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentComputeResourceImageViews.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentComputeResources[stage] = nullptr;
                            context->currentComputeResourceImageViews[stage] = VK_NULL_HANDLE;
                            context->currentComputeResourceSamplers[stage] = VK_NULL_HANDLE;
                            context->currentComputeResourceBuffers[stage] = nullptr;
                        }
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentComputeUnorderedAccessImageViews.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentComputeUnorderedAccessImageViews.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentComputeUnorderedAccessResources[stage] = nullptr;
                            context->currentComputeUnorderedAccessImageViews[stage] = VK_NULL_HANDLE;
                            context->currentComputeUnorderedAccessBuffers[stage] = nullptr;
                        }
                    }
                };

                class VertexPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;

                  public:
                    VertexPipeline(Context *context)
                        : context(context)
                    {
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Vertex;
                    }

                    void setProgram(Render::Program *program)
                    {
                        auto *vertexProgram = getObject<VertexProgram>(program);
                        context->pipelineDevice->trackVertexProgramSet((program != nullptr), (vertexProgram != nullptr));

                        context->currentVertexProgram = vertexProgram;
                    }

                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t bufferIndex = 0; bufferIndex < list.size(); ++bufferIndex)
                        {
                            const uint32_t slot = firstStage + bufferIndex;
                            if (slot >= context->currentVertexConstantBuffers.size())
                            {
                                continue;
                            }

                            context->currentVertexConstantBuffers[slot] = getObject<Buffer>(list[bufferIndex]);
                        }
                    }

                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(false && "Vertex pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentVertexConstantBuffers.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentVertexConstantBuffers.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentVertexConstantBuffers[stage] = nullptr;
                        }
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(false && "Vertex pipeline does not supported unordered access");
                    }
                };

                class GeometryPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;

                  public:
                    GeometryPipeline(Context *context)
                        : context(context)
                    {
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Geometry;
                    }

                    void setProgram(Render::Program *program)
                    {
                    }

                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                    }

                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(false && "Geometry pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(false && "Geometry pipeline does not supported unordered access");
                    }
                };

                class PixelPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;

                  public:
                    PixelPipeline(Context *context)
                        : context(context)
                    {
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Pixel;
                    }

                    void setProgram(Render::Program *program)
                    {
                        auto *pixelProgram = getObject<PixelProgram>(program);
                        context->pipelineDevice->trackPixelProgramSet((program != nullptr), (pixelProgram != nullptr));

                        context->currentPixelProgram = pixelProgram;
                    }

                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t samplerIndex = 0; samplerIndex < list.size(); ++samplerIndex)
                        {
                            const uint32_t slot = firstStage + samplerIndex;
                            if (slot >= context->currentPixelSamplerStates.size())
                            {
                                continue;
                            }

                            auto samplerState = getObject<SamplerState>(list[samplerIndex]);
                            context->currentPixelSamplerStates[slot] = (samplerState ? samplerState->sampler : VK_NULL_HANDLE);
                        }
                    }

                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t bufferIndex = 0; bufferIndex < list.size(); ++bufferIndex)
                        {
                            const uint32_t slot = firstStage + bufferIndex;
                            if (slot >= context->currentPixelConstantBuffers.size())
                            {
                                continue;
                            }

                            context->currentPixelConstantBuffers[slot] = getObject<Buffer>(list[bufferIndex]);
                        }
                    }

                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        if (list.empty())
                        {
                            return;
                        }

                        for (uint32_t resourceIndex = 0; resourceIndex < list.size(); ++resourceIndex)
                        {
                            const uint32_t slot = firstStage + resourceIndex;
                            if (slot >= context->currentPixelResourceImageViews.size())
                            {
                                continue;
                            }

                            auto *resource = list[resourceIndex];
                            VkImageView imageView = VK_NULL_HANDLE;
                            VkSampler imageSampler = VK_NULL_HANDLE;
                            if (auto viewTexture = getObject<ViewTexture>(resource))
                            {
                                imageView = viewTexture->imageView;
                                imageSampler = viewTexture->sampler;
                                context->currentPixelResourceBuffers[slot] = nullptr;
                            }
                            else if (auto targetTexture = getObject<TargetTexture>(resource))
                            {
                                imageView = targetTexture->imageView;
                                imageSampler = targetTexture->sampler;
                                context->currentPixelResourceBuffers[slot] = nullptr;
                            }
                            else if (auto depthTexture = getObject<DepthTexture>(resource))
                            {
                                imageView = depthTexture->imageView;
                                imageSampler = depthTexture->sampler;
                                context->currentPixelResourceBuffers[slot] = nullptr;
                            }
                            else if (auto resourceBuffer = getObject<Buffer>(resource))
                            {
                                context->currentPixelResourceBuffers[slot] = resourceBuffer;
                                imageView = VK_NULL_HANDLE;
                                imageSampler = VK_NULL_HANDLE;
                            }
                            else
                            {
                                context->currentPixelResourceBuffers[slot] = nullptr;
                            }

                            context->currentPixelResourceImageViews[slot] = imageView;
                            context->currentPixelResourceSamplers[slot] = imageSampler;
                        }
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentPixelSamplerStates.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentPixelSamplerStates.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentPixelSamplerStates[stage] = VK_NULL_HANDLE;
                        }
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentPixelConstantBuffers.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentPixelConstantBuffers.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentPixelConstantBuffers[stage] = nullptr;
                        }
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        if (count == 0 || firstStage >= context->currentPixelResourceImageViews.size())
                        {
                            return;
                        }

                        const uint32_t endStage = std::min<uint32_t>(static_cast<uint32_t>(context->currentPixelResourceImageViews.size()), firstStage + count);
                        for (uint32_t stage = firstStage; stage < endStage; ++stage)
                        {
                            context->currentPixelResourceImageViews[stage] = VK_NULL_HANDLE;
                            context->currentPixelResourceSamplers[stage] = VK_NULL_HANDLE;
                            context->currentPixelResourceBuffers[stage] = nullptr;
                        }
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                    }
                };

              public:
                Device *pipelineDevice = nullptr;
                VkDevice device;
                bool isDeferredContext = false;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;

                InputLayout *currentInputLayout = nullptr;
                std::array<Buffer *, 8> currentVertexBufferList{};
                std::array<uint32_t, 8> currentVertexBufferOffsetList{};
                Buffer *currentIndexBuffer = nullptr;
                uint32_t currentIndexBufferOffset = 0;
                Render::PrimitiveType currentPrimitiveType = Render::PrimitiveType::TriangleList;
                VkRect2D currentScissor = { { 0, 0 }, { 0, 0 } };
                TargetTexture *currentRenderTarget = nullptr;
                std::array<TargetTexture *, 8> currentRenderTargetList{};
                uint32_t currentRenderTargetCount = 0;
                DepthTexture *currentDepthTarget = nullptr;

                VertexProgram *currentVertexProgram = nullptr;
                PixelProgram *currentPixelProgram = nullptr;
                ComputeProgram *currentComputeProgram = nullptr;
                std::array<Buffer *, ContextResourceSlotCount> currentVertexConstantBuffers{};
                std::array<Buffer *, ContextResourceSlotCount> currentPixelConstantBuffers{};
                std::array<Buffer *, ContextResourceSlotCount> currentComputeConstantBuffers{};
                std::array<VkImageView, 16> currentPixelResourceImageViews{};
                std::array<VkSampler, 16> currentPixelResourceSamplers{};
                std::array<VkSampler, 16> currentPixelSamplerStates{};
                std::array<Buffer *, ContextResourceSlotCount> currentPixelResourceBuffers{};
                std::array<Render::Object *, ContextResourceSlotCount> currentComputeResources{};
                std::array<VkImageView, 16> currentComputeResourceImageViews{};
                std::array<VkSampler, 16> currentComputeResourceSamplers{};
                std::array<Buffer *, ContextResourceSlotCount> currentComputeResourceBuffers{};
                std::array<Render::Object *, ContextResourceSlotCount> currentComputeUnorderedAccessResources{};
                std::array<VkImageView, 16> currentComputeUnorderedAccessImageViews{};
                std::array<Buffer *, ContextResourceSlotCount> currentComputeUnorderedAccessBuffers{};
                BlendState *currentBlendState = nullptr;
                DepthState *currentDepthState = nullptr;
                RenderState *currentRenderState = nullptr;

              public:
                Context(Device *pipelineDevice, bool isDeferredContext = false)
                    : pipelineDevice(pipelineDevice), isDeferredContext(isDeferredContext), computeSystemHandler(new ComputePipeline(this)), vertexSystemHandler(new VertexPipeline(this)), geomtrySystemHandler(new GeometryPipeline(this)), pixelSystemHandler(new PixelPipeline(this))
                {
                    assert(computeSystemHandler);
                    assert(vertexSystemHandler);
                    assert(geomtrySystemHandler);
                    assert(pixelSystemHandler);
                }

                // Render::Context
                Pipeline *const computePipeline(void)
                {
                    assert(computeSystemHandler);

                    return computeSystemHandler.get();
                }

                Pipeline *const vertexPipeline(void)
                {
                    assert(vertexSystemHandler);

                    return vertexSystemHandler.get();
                }

                Pipeline *const geometryPipeline(void)
                {
                    assert(geomtrySystemHandler);

                    return geomtrySystemHandler.get();
                }

                Pipeline *const pixelPipeline(void)
                {
                    assert(pixelSystemHandler);

                    return pixelSystemHandler.get();
                }

                void begin(Render::Query *query)
                {
                }

                void end(Render::Query *query)
                {
                }

                Render::Query::Status getData(Render::Query *query, void *data, size_t dataSize, bool waitUntilReady = false)
                {
                    return Render::Query::Status::Error;
                }

                void generateMipMaps(Render::Texture *texture)
                {
                    if (!pipelineDevice || !texture)
                    {
                        return;
                    }

                    pipelineDevice->enqueueGenerateMipMapsCommand(this, texture);
                }

                void resolveSamples(Render::Texture *destination, Render::Texture *source)
                {
                }

                void copyResource(Render::Object *destination, Render::Object *source)
                {
                    if (!pipelineDevice || !destination || !source)
                    {
                        return;
                    }

                    pipelineDevice->enqueueCopyResourceCommand(this, destination, source);
                }

                void clearState(void)
                {
                    currentInputLayout = nullptr;
                    currentVertexBufferList.fill(nullptr);
                    currentVertexBufferOffsetList.fill(0);
                    currentIndexBuffer = nullptr;
                    currentScissor = { { 0, 0 }, { 0, 0 } };
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                    currentDepthTarget = nullptr;
                    currentVertexProgram = nullptr;
                    currentPixelProgram = nullptr;
                    currentComputeProgram = nullptr;
                    currentVertexConstantBuffers.fill(nullptr);
                    currentPixelConstantBuffers.fill(nullptr);
                    currentComputeConstantBuffers.fill(nullptr);
                    currentPixelResourceImageViews.fill(VK_NULL_HANDLE);
                    currentPixelResourceSamplers.fill(VK_NULL_HANDLE);
                    currentPixelSamplerStates.fill(VK_NULL_HANDLE);
                    currentPixelResourceBuffers.fill(nullptr);
                    currentComputeResources.fill(nullptr);
                    currentComputeResourceImageViews.fill(VK_NULL_HANDLE);
                    currentComputeResourceSamplers.fill(VK_NULL_HANDLE);
                    currentComputeResourceBuffers.fill(nullptr);
                    currentComputeUnorderedAccessResources.fill(nullptr);
                    currentComputeUnorderedAccessImageViews.fill(VK_NULL_HANDLE);
                    currentComputeUnorderedAccessBuffers.fill(nullptr);
                    currentBlendState = nullptr;
                    currentDepthState = nullptr;
                }

                void setViewportList(const std::vector<Render::ViewPort> &viewPortList)
                {
                    if (pipelineDevice && !viewPortList.empty())
                    {
                        const auto &viewPort = viewPortList[0];
                        pipelineDevice->currentViewport.x = viewPort.position.x;
                        pipelineDevice->currentViewport.y = viewPort.position.y;
                        pipelineDevice->currentViewport.width = viewPort.size.x;
                        pipelineDevice->currentViewport.height = viewPort.size.y;
                        pipelineDevice->currentViewport.minDepth = viewPort.nearClip;
                        pipelineDevice->currentViewport.maxDepth = viewPort.farClip;
                    }
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                    if (!rectangleList.empty())
                    {
                        const auto &rectangle = rectangleList[0];
                        currentScissor.offset.x = static_cast<int32_t>(rectangle.minimum.x);
                        currentScissor.offset.y = static_cast<int32_t>(rectangle.minimum.y);
                        currentScissor.extent.width = std::max(rectangle.maximum.x, rectangle.minimum.x) - rectangle.minimum.x;
                        currentScissor.extent.height = std::max(rectangle.maximum.y, rectangle.minimum.y) - rectangle.minimum.y;
                    }
                }

                void clearResource(Render::Object *object, Math::Float4 const &value)
                {
                }

                void clearUnorderedAccess(Render::Object *object, Math::Float4 const &value)
                {
                }

                void clearUnorderedAccess(Render::Object *object, Math::UInt4 const &value)
                {
                }

                void clearRenderTarget(Render::Target *renderTarget, Math::Float4 const &clearColor)
                {
                    if (!pipelineDevice || !renderTarget)
                    {
                        return;
                    }

                    if (renderTarget == pipelineDevice->backBuffer.get())
                    {
                        pipelineDevice->pendingClearColor = {
                            clearColor.r,
                            clearColor.g,
                            clearColor.b,
                            clearColor.a,
                        };
                        return;
                    }

                    pipelineDevice->enqueueClearRenderTargetCommand(this, renderTarget, clearColor);
                }

                void clearDepthStencilTarget(Render::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    if (!pipelineDevice || !depthBuffer || flags == 0)
                    {
                        return;
                    }

                    pipelineDevice->enqueueClearDepthStencilCommand(this, depthBuffer, flags, clearDepth, clearStencil);
                }

                void clearIndexBuffer(void)
                {
                    currentIndexBuffer = nullptr;
                    currentIndexBufferOffset = 0;
                }

                void clearVertexBufferList(uint32_t count, uint32_t firstSlot)
                {
                    if (firstSlot >= currentVertexBufferList.size())
                    {
                        return;
                    }

                    const uint32_t endSlot = std::min<uint32_t>(static_cast<uint32_t>(currentVertexBufferList.size()), firstSlot + count);
                    for (uint32_t slot = firstSlot; slot < endSlot; ++slot)
                    {
                        currentVertexBufferList[slot] = nullptr;
                        currentVertexBufferOffsetList[slot] = 0;
                    }
                }

                void clearRenderTargetList(uint32_t count, bool depthBuffer)
                {
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                    if (depthBuffer)
                    {
                        currentDepthTarget = nullptr;
                    }
                }

                void setRenderTargetList(const std::vector<Render::Target *> &renderTargetList, Render::Object *depthBuffer)
                {
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                    currentDepthTarget = getObject<DepthTexture>(depthBuffer);
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    ++pipelineDevice->frameRenderTargetBindCount;

                    for (auto *renderTarget : renderTargetList)
                    {
                        if (!renderTarget || renderTarget == pipelineDevice->backBuffer.get())
                        {
                            continue;
                        }

                        auto *targetTexture = getObject<TargetTexture>(renderTarget);
                        if (!targetTexture)
                        {
                            continue;
                        }

                        if (currentRenderTargetCount < currentRenderTargetList.size())
                        {
                            currentRenderTargetList[currentRenderTargetCount++] = targetTexture;
                        }
                    }

                    if (currentRenderTargetCount > 0)
                    {
                        ++pipelineDevice->frameOffscreenTargetBindCount;
                    }

                    currentRenderTarget = (currentRenderTargetCount > 0) ? currentRenderTargetList[0] : nullptr;
                }

                void setRenderState(Render::Object *renderState)
                {
                    currentRenderState = getObject<RenderState>(renderState);
                }

                void setDepthState(Render::Object *depthState, uint32_t stencilReference)
                {
                    currentDepthState = getObject<DepthState>(depthState);
                }

                void setBlendState(Render::Object *blendState, Math::Float4 const &blendFactor, uint32_t mask)
                {
                    currentBlendState = getObject<BlendState>(blendState);
                }

                void setInputLayout(Render::Object *inputLayout)
                {
                    currentInputLayout = getObject<InputLayout>(inputLayout);
                }

                void setIndexBuffer(Render::Buffer *indexBuffer, uint32_t offset)
                {
                    currentIndexBuffer = getObject<Buffer>(indexBuffer);
                    currentIndexBufferOffset = offset;
                }

                void setVertexBufferList(const std::vector<Render::Buffer *> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                    if (vertexBufferList.empty())
                    {
                        clearVertexBufferList(static_cast<uint32_t>(currentVertexBufferList.size()), 0);
                        return;
                    }

                    clearVertexBufferList(static_cast<uint32_t>(vertexBufferList.size()), firstSlot);

                    for (uint32_t bufferIndex = 0; bufferIndex < vertexBufferList.size(); ++bufferIndex)
                    {
                        const uint32_t slot = firstSlot + bufferIndex;
                        if (slot >= currentVertexBufferList.size())
                        {
                            continue;
                        }

                        currentVertexBufferList[slot] = getObject<Buffer>(vertexBufferList[bufferIndex]);
                        currentVertexBufferOffsetList[slot] = (offsetList ? offsetList[bufferIndex] : 0);
                    }
                }

                void setPrimitiveType(Render::PrimitiveType primitiveType)
                {
                    currentPrimitiveType = primitiveType;
                }

                void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++pipelineDevice->frameCapturedDiscardedNoProgramCount;
                        if (!currentVertexProgram && !currentPixelProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexOrPixelProgramCount;
                        }
                        else if (!currentVertexProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexProgramCount;
                        }
                        else
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoPixelProgramCount;
                        }
                        return;
                    }

                    if (vertexCount == 0)
                    {
                        return;
                    }

                    DrawCommand command;
                    command.indexed = false;
                    command.instanceCount = 1;
                    command.firstInstance = 0;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.vertexCount = vertexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = pipelineDevice->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelSamplerStates = currentPixelSamplerStates;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = (currentPixelResourceImageViews.empty() ? VK_NULL_HANDLE : currentPixelResourceImageViews[0]);
                    command.pixelSampler = (currentPixelSamplerStates.empty() ? VK_NULL_HANDLE : currentPixelSamplerStates[0]);
                    command.renderTarget = currentRenderTarget;
                    command.depthTarget = currentDepthTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = std::min<uint32_t>(currentRenderTargetCount, static_cast<uint32_t>(command.offscreenImages.size()));
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    ++pipelineDevice->frameCapturedDrawCommandCount;
                    if (command.hasOffscreenTarget)
                    {
                        ++pipelineDevice->frameCapturedOffscreenDrawCommandCount;
                    }
                    else
                    {
                        ++pipelineDevice->frameCapturedBackbufferDrawCommandCount;
                        if (pipelineDevice->frameOffscreenTargetBindCount > 0)
                        {
                            ++pipelineDevice->frameCapturedBackbufferAfterOffscreenBindCount;
                        }
                    }

                    std::lock_guard<std::recursive_mutex> lock(Device::getDrawCommandMutex());
                    for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                    {
                        command.vertexConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.vertexConstantBuffers[slot], false);
                        command.pixelConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.pixelConstantBuffers[slot], false);
                        command.vertexConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexConstantBuffers[slot]);
                        command.pixelConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.pixelConstantBuffers[slot]);
                    }

                    command.indexBufferVersion = pipelineDevice->captureVersionedBufferSlot(command.indexBuffer);
                    for (uint32_t slot = 0; slot < command.vertexBuffers.size(); ++slot)
                    {
                        command.vertexBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexBuffers[slot]);
                    }

                    for (uint32_t targetIndex = 0; targetIndex < command.offscreenTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = (target->actualFormat != VK_FORMAT_UNDEFINED) ? target->actualFormat : GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    pipelineDevice->recordCommand(command);
                    ++pipelineDevice->frameTotalCommandCount;
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++pipelineDevice->frameCapturedDiscardedNoProgramCount;
                        if (!currentVertexProgram && !currentPixelProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexOrPixelProgramCount;
                        }
                        else if (!currentVertexProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexProgramCount;
                        }
                        else
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoPixelProgramCount;
                        }
                        return;
                    }

                    if (vertexCount == 0 || instanceCount == 0)
                    {
                        return;
                    }

                    DrawCommand command;
                    command.indexed = false;
                    command.instanceCount = instanceCount;
                    command.firstInstance = firstInstance;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.vertexCount = vertexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = pipelineDevice->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelSamplerStates = currentPixelSamplerStates;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = (currentPixelResourceImageViews.empty() ? VK_NULL_HANDLE : currentPixelResourceImageViews[0]);
                    command.pixelSampler = (currentPixelSamplerStates.empty() ? VK_NULL_HANDLE : currentPixelSamplerStates[0]);
                    command.renderTarget = currentRenderTarget;
                    command.depthTarget = currentDepthTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = std::min<uint32_t>(currentRenderTargetCount, static_cast<uint32_t>(command.offscreenImages.size()));
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    ++pipelineDevice->frameCapturedDrawCommandCount;
                    if (command.hasOffscreenTarget)
                    {
                        ++pipelineDevice->frameCapturedOffscreenDrawCommandCount;
                    }
                    else
                    {
                        ++pipelineDevice->frameCapturedBackbufferDrawCommandCount;
                        if (pipelineDevice->frameOffscreenTargetBindCount > 0)
                        {
                            ++pipelineDevice->frameCapturedBackbufferAfterOffscreenBindCount;
                        }
                    }

                    std::lock_guard<std::recursive_mutex> lock(Device::getDrawCommandMutex());
                    for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                    {
                        command.vertexConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.vertexConstantBuffers[slot], false);
                        command.pixelConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.pixelConstantBuffers[slot], false);
                        command.vertexConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexConstantBuffers[slot]);
                        command.pixelConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.pixelConstantBuffers[slot]);
                    }

                    command.indexBufferVersion = pipelineDevice->captureVersionedBufferSlot(command.indexBuffer);
                    for (uint32_t slot = 0; slot < command.vertexBuffers.size(); ++slot)
                    {
                        command.vertexBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexBuffers[slot]);
                    }

                    for (uint32_t targetIndex = 0; targetIndex < command.offscreenTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = (target->actualFormat != VK_FORMAT_UNDEFINED) ? target->actualFormat : GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    pipelineDevice->recordCommand(command);
                    ++pipelineDevice->frameTotalCommandCount;
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++pipelineDevice->frameCapturedDiscardedNoProgramCount;
                        if (!currentVertexProgram && !currentPixelProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexOrPixelProgramCount;
                        }
                        else if (!currentVertexProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexProgramCount;
                        }
                        else
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoPixelProgramCount;
                        }
                        return;
                    }

                    if (currentVertexBufferList.empty())
                    {
                        return;
                    }

                    if (!currentIndexBuffer)
                    {
                        return;
                    }

                    if (indexCount == 0)
                    {
                        return;
                    }

                    DrawCommand command;
                    command.indexed = true;
                    command.instanceCount = 1;
                    command.firstInstance = 0;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.indexBuffer = currentIndexBuffer;
                    command.indexOffset = currentIndexBufferOffset + firstIndex * (currentIndexBuffer->getDescription().format == Render::Format::R16_UINT ? 2u : 4u);
                    command.indexCount = indexCount;
                    command.firstVertex = firstVertex;
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = pipelineDevice->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelSamplerStates = currentPixelSamplerStates;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = (currentPixelResourceImageViews.empty() ? VK_NULL_HANDLE : currentPixelResourceImageViews[0]);
                    command.pixelSampler = (currentPixelSamplerStates.empty() ? VK_NULL_HANDLE : currentPixelSamplerStates[0]);
                    command.renderTarget = currentRenderTarget;
                    command.depthTarget = currentDepthTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = std::min<uint32_t>(currentRenderTargetCount, static_cast<uint32_t>(command.offscreenImages.size()));
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    ++pipelineDevice->frameCapturedDrawCommandCount;
                    if (command.hasOffscreenTarget)
                    {
                        ++pipelineDevice->frameCapturedOffscreenDrawCommandCount;
                    }
                    else
                    {
                        ++pipelineDevice->frameCapturedBackbufferDrawCommandCount;
                        if (pipelineDevice->frameOffscreenTargetBindCount > 0)
                        {
                            ++pipelineDevice->frameCapturedBackbufferAfterOffscreenBindCount;
                        }
                    }

                    std::lock_guard<std::recursive_mutex> lock(Device::getDrawCommandMutex());
                    for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                    {
                        command.vertexConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.vertexConstantBuffers[slot], false);
                        command.pixelConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.pixelConstantBuffers[slot], false);
                        command.vertexConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexConstantBuffers[slot]);
                        command.pixelConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.pixelConstantBuffers[slot]);
                    }

                    command.indexBufferVersion = pipelineDevice->captureVersionedBufferSlot(command.indexBuffer);
                    for (uint32_t slot = 0; slot < command.vertexBuffers.size(); ++slot)
                    {
                        command.vertexBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexBuffers[slot]);
                    }

                    for (uint32_t targetIndex = 0; targetIndex < command.offscreenTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = (target->actualFormat != VK_FORMAT_UNDEFINED) ? target->actualFormat : GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    pipelineDevice->recordCommand(command);
                    ++pipelineDevice->frameTotalCommandCount;
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++pipelineDevice->frameCapturedDiscardedNoProgramCount;
                        if (!currentVertexProgram && !currentPixelProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexOrPixelProgramCount;
                        }
                        else if (!currentVertexProgram)
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoVertexProgramCount;
                        }
                        else
                        {
                            ++pipelineDevice->frameCapturedDiscardedNoPixelProgramCount;
                        }
                        return;
                    }

                    if (currentVertexBufferList.empty())
                    {
                        return;
                    }

                    if (!currentIndexBuffer)
                    {
                        return;
                    }

                    if (indexCount == 0 || instanceCount == 0)
                    {
                        return;
                    }

                    DrawCommand command;
                    command.indexed = true;
                    command.instanceCount = instanceCount;
                    command.firstInstance = firstInstance;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.indexBuffer = currentIndexBuffer;
                    command.indexOffset = currentIndexBufferOffset + firstIndex * (currentIndexBuffer->getDescription().format == Render::Format::R16_UINT ? 2u : 4u);
                    command.indexCount = indexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = pipelineDevice->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelSamplerStates = currentPixelSamplerStates;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = (currentPixelResourceImageViews.empty() ? VK_NULL_HANDLE : currentPixelResourceImageViews[0]);
                    command.pixelSampler = (currentPixelSamplerStates.empty() ? VK_NULL_HANDLE : currentPixelSamplerStates[0]);
                    command.renderTarget = currentRenderTarget;
                    command.depthTarget = currentDepthTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = std::min<uint32_t>(currentRenderTargetCount, static_cast<uint32_t>(command.offscreenImages.size()));
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    ++pipelineDevice->frameCapturedDrawCommandCount;
                    if (command.hasOffscreenTarget)
                    {
                        ++pipelineDevice->frameCapturedOffscreenDrawCommandCount;
                    }
                    else
                    {
                        ++pipelineDevice->frameCapturedBackbufferDrawCommandCount;
                        if (pipelineDevice->frameOffscreenTargetBindCount > 0)
                        {
                            ++pipelineDevice->frameCapturedBackbufferAfterOffscreenBindCount;
                        }
                    }

                    std::lock_guard<std::recursive_mutex> lock(Device::getDrawCommandMutex());
                    for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                    {
                        command.vertexConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.vertexConstantBuffers[slot], false);
                        command.pixelConstantBuffers[slot] = pipelineDevice->captureBufferSnapshot(command.pixelConstantBuffers[slot], false);
                        command.vertexConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexConstantBuffers[slot]);
                        command.pixelConstantBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.pixelConstantBuffers[slot]);
                    }

                    command.indexBufferVersion = pipelineDevice->captureVersionedBufferSlot(command.indexBuffer);
                    for (uint32_t slot = 0; slot < command.vertexBuffers.size(); ++slot)
                    {
                        command.vertexBufferVersions[slot] = pipelineDevice->captureVersionedBufferSlot(command.vertexBuffers[slot]);
                    }

                    for (uint32_t targetIndex = 0; targetIndex < command.offscreenTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = (target->actualFormat != VK_FORMAT_UNDEFINED) ? target->actualFormat : GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    pipelineDevice->recordCommand(command);
                    ++pipelineDevice->frameTotalCommandCount;
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentComputeProgram)
                    {
                        pipelineDevice->getContext()->log(Gek::Context::Warning, "Vulkan compute dispatch skipped: no compute program bound");
                        return;
                    }

                    if (threadGroupCountX == 0 || threadGroupCountY == 0 || threadGroupCountZ == 0)
                    {
                        pipelineDevice->getContext()->log(Gek::Context::Warning, "Vulkan compute dispatch skipped: zero thread group dimension ({}, {}, {})", threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                        return;
                    }

                    pipelineDevice->enqueueComputeDispatchCommand(this, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }

                Render::ObjectPtr finishCommandList(void)
                {
                    if (!pipelineDevice)
                    {
                        return nullptr;
                    }

                    auto commandList = std::make_unique<CommandList>();
                    commandList->identifier = pipelineDevice->nextCommandListIdentifier++;
                    if (isDeferredContext)
                    {
                        std::lock_guard<std::recursive_mutex> lock(Device::getDrawCommandMutex());
                        auto deferredContextCommandIterator = pipelineDevice->deferredContextDrawCommands.find(this);
                        if (deferredContextCommandIterator != pipelineDevice->deferredContextDrawCommands.end())
                        {
                            pipelineDevice->deferredCommandLists[commandList->identifier] = std::move(deferredContextCommandIterator->second);
                            pipelineDevice->deferredContextDrawCommands.erase(deferredContextCommandIterator);
                        }
                    }

                    return commandList;
                }
            };

          public:
            Window::Device *window = nullptr;
            Render::Device::ContextPtr defaultContext;
            Render::TargetPtr backBuffer;

            bool enableValidationLayer = false;
            VkDebugUtilsMessengerEXT debugMessenger;

            bool kronosBaseSurfaceAvailable = false;
            bool kronosWin32SurfaceAvailable = false;
            bool kronosMacOSSurfaceAvailable = false;
            bool kronosX11SurfaceAvailable = false;
            bool kronosX11CBSurfaceAvailable = false;

            VkInstance instance = VK_NULL_HANDLE;
            VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
            VkDevice device = VK_NULL_HANDLE;
            VkDisplayKHR display = VK_NULL_HANDLE;

            VkQueue graphicsQueue = VK_NULL_HANDLE;
            VkQueue presentQueue = VK_NULL_HANDLE;

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            VkSwapchainKHR swapChain = VK_NULL_HANDLE;
            VkFormat swapChainImageFormat;
            VkExtent2D swapChainExtent;
            std::vector<VkImage> swapChainImages;
            std::vector<VkImageView> swapChainImageViews;
            std::vector<VkImageLayout> swapChainImageLayouts;
            VkImage depthImage = VK_NULL_HANDLE;
            VkImageLayout depthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkDeviceMemory depthMemory = VK_NULL_HANDLE;
            VkImageView depthImageView = VK_NULL_HANDLE;
            VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
            VkCommandPool commandPool = VK_NULL_HANDLE;
            VkCommandPool uploadCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
            std::vector<VkSemaphore> renderFinishedSemaphores;
            VkFence inFlightFence = VK_NULL_HANDLE;
            bool inFlightFencePending = false;
            VkRenderPass renderPass = VK_NULL_HANDLE;
            std::vector<VkFramebuffer> swapChainFramebuffers;
            std::map<std::pair<std::vector<VkFormat>, VkFormat>, VkRenderPass> offscreenRenderPassCache;
            std::vector<VkFramebuffer> transientFramebuffers;

            VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
            VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
            VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
            static constexpr uint32_t PixelResourceSlotCount = 16;
            static constexpr uint32_t DescriptorSampledImageBase = 0;
            static constexpr uint32_t DescriptorStorageBufferBase = 64;
            static constexpr uint32_t DescriptorVertexUniformBufferBase = 128;
            static constexpr uint32_t DescriptorPixelUniformBufferBase = 144;
            static constexpr uint32_t DescriptorSamplerBase = 192;
            static constexpr uint32_t DescriptorStorageImageBase = 224;

            struct DrawCommand
            {
                enum class Type
                {
                    Draw,
                    ComputeDispatch,
                    GenerateMipMaps,
                    ClearRenderTarget,
                    ClearDepthStencil,
                    CopyResource,
                };

                Type commandType = Type::Draw;
                bool indexed = false;
                uint32_t instanceCount = 1;
                uint32_t firstInstance = 0;
                std::array<Buffer *, 8> vertexBuffers{};
                std::array<uint8_t, 8> vertexBufferVersions{};
                std::array<uint32_t, 8> vertexOffsets{};
                uint32_t vertexCount = 0;
                Buffer *indexBuffer = nullptr;
                uint8_t indexBufferVersion = 0;
                uint32_t indexOffset = 0;
                uint32_t indexCount = 0;
                int32_t firstVertex = 0;
                Render::PrimitiveType primitiveType = Render::PrimitiveType::TriangleList;
                VkRect2D scissor = { { 0, 0 }, { 1, 1 } };
                VkViewport viewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
                InputLayout *inputLayout = nullptr;
                VertexProgram *vertexProgram = nullptr;
                PixelProgram *pixelProgram = nullptr;
                std::array<Buffer *, PixelResourceSlotCount> vertexConstantBuffers{};
                std::array<Buffer *, PixelResourceSlotCount> pixelConstantBuffers{};
                std::array<uint8_t, PixelResourceSlotCount> vertexConstantBufferVersions{};
                std::array<uint8_t, PixelResourceSlotCount> pixelConstantBufferVersions{};
                VkImageView pixelImageView = VK_NULL_HANDLE;
                VkSampler pixelSampler = VK_NULL_HANDLE;
                std::array<VkImageView, PixelResourceSlotCount> pixelResourceImageViews{};
                std::array<VkSampler, PixelResourceSlotCount> pixelResourceSamplers{};
                std::array<VkSampler, PixelResourceSlotCount> pixelSamplerStates{};
                std::array<Buffer *, PixelResourceSlotCount> pixelResourceBuffers{};
                TargetTexture *renderTarget = nullptr;
                DepthTexture *depthTarget = nullptr;
                bool hasOffscreenTarget = false;
                uint32_t offscreenTargetCount = 0;
                std::array<VkImage, 8> offscreenImages{};
                std::array<VkImageView, 8> offscreenImageViews{};
                std::array<VkFormat, 8> offscreenFormats{};
                std::array<VkExtent2D, 8> offscreenExtents{};
                BlendState *blendState = nullptr;
                DepthState *depthState = nullptr;
                RenderState *renderState = nullptr;

                ComputeProgram *computeProgram = nullptr;
                uint32_t computeThreadGroupCountX = 0;
                uint32_t computeThreadGroupCountY = 0;
                uint32_t computeThreadGroupCountZ = 0;
                std::array<Buffer *, PixelResourceSlotCount> computeConstantBuffers{};
                std::array<uint8_t, PixelResourceSlotCount> computeConstantBufferVersions{};
                std::array<Render::Object *, PixelResourceSlotCount> computeResources{};
                std::array<VkImageView, PixelResourceSlotCount> computeResourceImageViews{};
                std::array<VkSampler, PixelResourceSlotCount> computeResourceSamplers{};
                std::array<Buffer *, PixelResourceSlotCount> computeResourceBuffers{};
                std::array<Render::Object *, PixelResourceSlotCount> computeUnorderedAccessResources{};
                std::array<VkImageView, PixelResourceSlotCount> computeUnorderedAccessImageViews{};
                std::array<Buffer *, PixelResourceSlotCount> computeUnorderedAccessBuffers{};

                Render::Texture *mipmapTexture = nullptr;
                uint32_t mipmapLevels = 1;

                Render::Object *copyDestination = nullptr;
                Render::Object *copySource = nullptr;

                TargetTexture *clearRenderTarget = nullptr;
                VkClearColorValue clearRenderTargetColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };

                DepthTexture *clearDepthTarget = nullptr;
                uint32_t clearDepthStencilFlags = 0;
                float clearDepthValue = 1.0f;
                uint32_t clearStencilValue = 0;
            };

            std::map<Context *, std::vector<DrawCommand>> deferredContextDrawCommands;
            std::map<uint64_t, std::vector<DrawCommand>> deferredCommandLists;

            // Per-frame immediate-recording state
            struct GraphicsDescriptorSignature
            {
                std::array<VkImageView, PixelResourceSlotCount> pixelResourceImageViews{};
                std::array<VkSampler, PixelResourceSlotCount> pixelSamplerStates{};
                std::array<Buffer *, PixelResourceSlotCount> pixelResourceBuffers{};
                std::array<Buffer *, PixelResourceSlotCount> pixelConstantBuffers{};
                std::array<Buffer *, PixelResourceSlotCount> vertexConstantBuffers{};

                bool operator==(const GraphicsDescriptorSignature &other) const
                {
                    return (pixelResourceImageViews == other.pixelResourceImageViews) &&
                           (pixelSamplerStates == other.pixelSamplerStates) &&
                           (pixelResourceBuffers == other.pixelResourceBuffers) &&
                           (pixelConstantBuffers == other.pixelConstantBuffers) &&
                           (vertexConstantBuffers == other.vertexConstantBuffers);
                }
            };

            bool frameRecordingActive = false;
            uint32_t frameImageIndex = 0;
            uint32_t frameTotalCommandCount = 0;
            uint32_t frameOffscreenDrawCount = 0;
            uint32_t frameBackbufferDrawCount = 0;
            bool frameBackbufferColorCleared = false;
            uint32_t frameOffscreenCommandCount = 0;
            uint32_t frameCapturedDrawCommandCount = 0;
            uint32_t frameCapturedOffscreenDrawCommandCount = 0;
            uint32_t frameCapturedBackbufferDrawCommandCount = 0;
            uint32_t frameCapturedBackbufferAfterOffscreenBindCount = 0;
            uint32_t frameCapturedDiscardedNoProgramCount = 0;
            uint32_t frameCapturedDiscardedNoVertexProgramCount = 0;
            uint32_t frameCapturedDiscardedNoPixelProgramCount = 0;
            uint32_t frameCapturedDiscardedNoVertexOrPixelProgramCount = 0;
            uint32_t frameVertexProgramSetCount = 0;
            uint32_t framePixelProgramSetCount = 0;
            uint32_t frameVertexProgramNullSetCount = 0;
            uint32_t framePixelProgramNullSetCount = 0;
            uint32_t frameVertexProgramTypeMismatchCount = 0;
            uint32_t framePixelProgramTypeMismatchCount = 0;
            uint32_t framePipelineFailCount = 0;
            uint32_t frameInvalidTargetCount = 0;
            uint32_t frameEmptyDescriptorCount = 0;
            uint32_t frameRenderTargetBindCount = 0;
            uint32_t frameOffscreenTargetBindCount = 0;
            uint32_t frameOffscreenSkippedNoTargetCount = 0;
            uint32_t frameOffscreenSkippedNullRenderPassCount = 0;
            uint32_t frameOffscreenSkippedNullFramebufferCount = 0;
            uint32_t frameBackbufferAccumulatePipelineFailCount = 0;
            uint32_t frameBackbufferAccumulateSkipCount = 0;
            uint64_t frameIndex = 0;
            std::vector<VkDescriptorSet> frameDescriptorSets;
            std::map<VkImageView, std::pair<VkImage, VkExtent2D>> frameOffscreenViewLookup;
            std::map<std::string, std::pair<VkImage, VkExtent2D>> frameNamedRenderTargetImages;
            VkImage frameSceneCopySourceImage = VK_NULL_HANDLE;
            VkExtent2D frameSceneCopySourceExtent = { 0, 0 };
            bool frameCompatibilitySceneCopyIssued = false;
            GraphicsDescriptorSignature frameLastDescriptorSignature{};
            bool frameHasLastDescriptor = false;
            VkDescriptorSet frameLastDescriptorSet = VK_NULL_HANDLE;

            Render::BufferVersioningPolicy constantBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            Render::BufferVersioningPolicy vertexBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            Render::BufferVersioningPolicy indexBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            uint64_t nextCommandListIdentifier = 1;
            std::set<Buffer *> versionedConstantBuffersInFlight;
            std::map<VkImage, VkImageLayout> offscreenImageLayouts;
            std::map<VkImageView, std::pair<VkImage, VkExtent2D>> persistentImageViewLookup;
            std::mutex persistentImageViewLookupMutex;

            struct PendingUploadSubmission
            {
                VkFence fence = VK_NULL_HANDLE;
                VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
                VkBuffer stagingBuffer = VK_NULL_HANDLE;
                VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
            };
            std::vector<PendingUploadSubmission> pendingUploadSubmissions;
            std::mutex pendingUploadSubmissionsMutex;

            uint64_t presentFrameIndex = 0;
            VkViewport currentViewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
            bool deviceLost = false;
            bool loggedDeviceLost = false;
            bool loggedOffscreenTargetCountZero = false;
            bool loggedOffscreenInvalidTargets = false;
            bool loggedOffscreenRenderPassNull = false;
            bool loggedOffscreenFramebufferNull = false;
            bool loggedCollapsedScissor = false;
            bool loggedGraphicsPipelineNull = false;
            bool loggedGraphicsPipelineShaderModuleNull = false;
            bool loggedOffscreenToColorTransition = false;
            bool loggedOffscreenToShaderReadTransition = false;
            bool samplerAnisotropySupported = false;
            float maxSamplerAnisotropy = 1.0f;
            bool preferSpirv13Profile = false;
            uint32_t lastLoggedPipelineFailCount = std::numeric_limits<uint32_t>::max();
            uint32_t lastLoggedInvalidTargetCount = std::numeric_limits<uint32_t>::max();

            void trackVertexProgramSet(bool hasProgram, bool typeMatched)
            {
                ++frameVertexProgramSetCount;
                if (!hasProgram)
                {
                    ++frameVertexProgramNullSetCount;
                }
                else if (!typeMatched)
                {
                    ++frameVertexProgramTypeMismatchCount;
                }
            }

            void trackPixelProgramSet(bool hasProgram, bool typeMatched)
            {
                ++framePixelProgramSetCount;
                if (!hasProgram)
                {
                    ++framePixelProgramNullSetCount;
                }
                else if (!typeMatched)
                {
                    ++framePixelProgramTypeMismatchCount;
                }
            }

            struct PipelineKey
            {
                VkShaderModule vertexModule = VK_NULL_HANDLE;
                VkShaderModule pixelModule = VK_NULL_HANDLE;
                InputLayout *inputLayout = nullptr;
                VkRenderPass renderPass = VK_NULL_HANDLE;
                Render::PrimitiveType primitiveType = Render::PrimitiveType::TriangleList;
                bool blendEnabled = false;
                bool depthEnabled = false;
                bool depthWrite = false;
                Render::ComparisonFunction depthCompareFunction = Render::ComparisonFunction::Always;
                Render::RenderState::CullMode cullMode = Render::RenderState::CullMode::None;
                bool frontCounterClockwise = false;

                bool operator<(const PipelineKey &other) const
                {
                    if (vertexModule != other.vertexModule)
                        return vertexModule < other.vertexModule;
                    if (pixelModule != other.pixelModule)
   