#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "API/System/RenderDevice.hpp"
#include "API/System/WindowDevice.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <execution>
#include <exception>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <array>
#include <utility>
#include <mutex>
#include <atomic>

#ifdef WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
    #include <ktx.h>
    #include <ktxvulkan.h>
    #include <objbase.h>
#endif

#include <vulkan/vulkan.h>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Gek
{
    namespace Render::Implementation
    {
        static std::atomic_bool gVulkanDeviceShuttingDown{ false };

        Render::Format GetFormat(VkFormat format)
        {
            switch (format)
            {
            case VK_FORMAT_R32G32B32A32_SFLOAT: return Render::Format::R32G32B32A32_FLOAT;
            case VK_FORMAT_R16G16B16A16_SFLOAT: return Render::Format::R16G16B16A16_FLOAT;
            case VK_FORMAT_R32G32B32_SFLOAT: return Render::Format::R32G32B32_FLOAT;
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return Render::Format::R11G11B10_FLOAT;
            case VK_FORMAT_R32G32_SFLOAT: return Render::Format::R32G32_FLOAT;
            case VK_FORMAT_R16G16_SFLOAT: return Render::Format::R16G16_FLOAT;
            case VK_FORMAT_R32_SFLOAT: return Render::Format::R32_FLOAT;
            case VK_FORMAT_R16_SFLOAT: return Render::Format::R16_FLOAT;

            case VK_FORMAT_R32G32B32A32_UINT: return Render::Format::R32G32B32A32_UINT;
            case VK_FORMAT_R16G16B16A16_UINT: return Render::Format::R16G16B16A16_UINT;
            //case VK_FORMAT_R10G10B10A2_UINT: return Render::Format::R10G10B10A2_UINT;
            case VK_FORMAT_R8G8B8A8_UINT: return Render::Format::R8G8B8A8_UINT;
            case VK_FORMAT_R32G32B32_UINT: return Render::Format::R32G32B32_UINT;
            case VK_FORMAT_R32G32_UINT: return Render::Format::R32G32_UINT;
            case VK_FORMAT_R16G16_UINT: return Render::Format::R16G16_UINT;
            case VK_FORMAT_R8G8_UINT: return Render::Format::R8G8_UINT;
            case VK_FORMAT_R32_UINT: return Render::Format::R32_UINT;
            case VK_FORMAT_R16_UINT: return Render::Format::R16_UINT;
            case VK_FORMAT_R8_UINT: return Render::Format::R8_UINT;

            case VK_FORMAT_R32G32B32A32_SINT: return Render::Format::R32G32B32A32_INT;
            case VK_FORMAT_R16G16B16A16_SINT: return Render::Format::R16G16B16A16_INT;
            case VK_FORMAT_R8G8B8A8_SINT: return Render::Format::R8G8B8A8_INT;
            case VK_FORMAT_R32G32B32_SINT: return Render::Format::R32G32B32_INT;
            case VK_FORMAT_R32G32_SINT: return Render::Format::R32G32_INT;
            case VK_FORMAT_R16G16_SINT: return Render::Format::R16G16_INT;
            case VK_FORMAT_R8G8_SINT: return Render::Format::R8G8_INT;
            case VK_FORMAT_R32_SINT: return Render::Format::R32_INT;
            case VK_FORMAT_R16_SINT: return Render::Format::R16_INT;
            case VK_FORMAT_R8_SINT: return Render::Format::R8_INT;

            case VK_FORMAT_D32_SFLOAT_S8_UINT: return Render::Format::D32_FLOAT_S8X24_UINT;
            case VK_FORMAT_D24_UNORM_S8_UINT: return Render::Format::D24_UNORM_S8_UINT;
            case VK_FORMAT_D32_SFLOAT: return Render::Format::D32_FLOAT;
            case VK_FORMAT_D16_UNORM: return Render::Format::D16_UNORM;

            case VK_FORMAT_R16G16B16A16_UNORM: return Render::Format::R16G16B16A16_UNORM;
            //case VK_FORMAT_R10G10B10A2_UNORM: return Render::Format::R10G10B10A2_UNORM;
            case VK_FORMAT_R8G8B8A8_UNORM: return Render::Format::R8G8B8A8_UNORM;
            case VK_FORMAT_R8G8B8A8_SRGB: return Render::Format::R8G8B8A8_UNORM_SRGB;
            case VK_FORMAT_R16G16_UNORM: return Render::Format::R16G16_UNORM;
            case VK_FORMAT_R8G8_UNORM: return Render::Format::R8G8_UNORM;
            case VK_FORMAT_R16_UNORM: return Render::Format::R16_UNORM;
            case VK_FORMAT_R8_UNORM: return Render::Format::R8_UNORM;

            case VK_FORMAT_R16G16B16A16_SNORM: return Render::Format::R16G16B16A16_NORM;
            case VK_FORMAT_R8G8B8A8_SNORM: return Render::Format::R8G8B8A8_NORM;
            case VK_FORMAT_R16G16_SNORM: return Render::Format::R16G16_NORM;
            case VK_FORMAT_R8G8_SNORM: return Render::Format::R8G8_NORM;
            case VK_FORMAT_R16_SNORM: return Render::Format::R16_NORM;
            case VK_FORMAT_R8_SNORM: return Render::Format::R8_NORM;
            };

            return Render::Format::Unknown;
        }

        VkFormat GetVkFormat(Render::Format format)
        {
            switch (format)
            {
            case Render::Format::R32G32B32A32_FLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Render::Format::R16G16B16A16_FLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Render::Format::R32G32B32_FLOAT: return VK_FORMAT_R32G32B32_SFLOAT;
            case Render::Format::R32G32_FLOAT: return VK_FORMAT_R32G32_SFLOAT;
            case Render::Format::R16G16_FLOAT: return VK_FORMAT_R16G16_SFLOAT;
            case Render::Format::R32_FLOAT: return VK_FORMAT_R32_SFLOAT;
            case Render::Format::R16_FLOAT: return VK_FORMAT_R16_SFLOAT;
            case Render::Format::R11G11B10_FLOAT: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

            case Render::Format::R32G32B32A32_UINT: return VK_FORMAT_R32G32B32A32_UINT;
            case Render::Format::R16G16B16A16_UINT: return VK_FORMAT_R16G16B16A16_UINT;
            case Render::Format::R8G8B8A8_UINT: return VK_FORMAT_R8G8B8A8_UINT;
            case Render::Format::R32G32B32_UINT: return VK_FORMAT_R32G32B32_UINT;
            case Render::Format::R32G32_UINT: return VK_FORMAT_R32G32_UINT;
            case Render::Format::R16G16_UINT: return VK_FORMAT_R16G16_UINT;
            case Render::Format::R8G8_UINT: return VK_FORMAT_R8G8_UINT;
            case Render::Format::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case Render::Format::R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;

            case Render::Format::R32G32B32A32_INT: return VK_FORMAT_R32G32B32A32_SINT;
            case Render::Format::R16G16B16A16_INT: return VK_FORMAT_R16G16B16A16_SINT;
            case Render::Format::R8G8B8A8_INT: return VK_FORMAT_R8G8B8A8_SINT;
            case Render::Format::R32G32B32_INT: return VK_FORMAT_R32G32B32_SINT;
            case Render::Format::R32G32_INT: return VK_FORMAT_R32G32_SINT;
            case Render::Format::R16G16_INT: return VK_FORMAT_R16G16_SINT;
            case Render::Format::R8G8_INT: return VK_FORMAT_R8G8_SINT;

            case Render::Format::R16G16_UNORM: return VK_FORMAT_R16G16_UNORM;
            case Render::Format::R8G8_UNORM: return VK_FORMAT_R8G8_UNORM;
            case Render::Format::R16_UNORM: return VK_FORMAT_R16_UNORM;
            case Render::Format::R8_UNORM: return VK_FORMAT_R8_UNORM;

            case Render::Format::R16G16_NORM: return VK_FORMAT_R16G16_SNORM;
            case Render::Format::R8G8_NORM: return VK_FORMAT_R8G8_SNORM;
            case Render::Format::R16_NORM: return VK_FORMAT_R16_SNORM;
            case Render::Format::R8_NORM: return VK_FORMAT_R8_SNORM;

            case Render::Format::R16_UINT: return VK_FORMAT_R16_UINT;
            case Render::Format::R32_UINT: return VK_FORMAT_R32_UINT;
            case Render::Format::D32_FLOAT_S8X24_UINT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case Render::Format::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
            case Render::Format::D32_FLOAT: return VK_FORMAT_D32_SFLOAT;
            case Render::Format::D16_UNORM: return VK_FORMAT_D16_UNORM;
            default:
                return VK_FORMAT_UNDEFINED;
            }
        }

        DXGI_FORMAT ResolveDxgiFormatForSrgbPreference(DXGI_FORMAT format, bool preferSrgb)
        {
            if (preferSrgb)
            {
                switch (format)
                {
                case DXGI_FORMAT_BC1_UNORM: return DXGI_FORMAT_BC1_UNORM_SRGB;
                case DXGI_FORMAT_BC2_UNORM: return DXGI_FORMAT_BC2_UNORM_SRGB;
                case DXGI_FORMAT_BC3_UNORM: return DXGI_FORMAT_BC3_UNORM_SRGB;
                case DXGI_FORMAT_BC7_UNORM: return DXGI_FORMAT_BC7_UNORM_SRGB;
                case DXGI_FORMAT_R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                default:
                    break;
                }
            }
            else
            {
                switch (format)
                {
                case DXGI_FORMAT_BC1_UNORM_SRGB: return DXGI_FORMAT_BC1_UNORM;
                case DXGI_FORMAT_BC2_UNORM_SRGB: return DXGI_FORMAT_BC2_UNORM;
                case DXGI_FORMAT_BC3_UNORM_SRGB: return DXGI_FORMAT_BC3_UNORM;
                case DXGI_FORMAT_BC7_UNORM_SRGB: return DXGI_FORMAT_BC7_UNORM;
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
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
            case DXGI_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case DXGI_FORMAT_BC1_UNORM: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case DXGI_FORMAT_BC1_UNORM_SRGB: return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case DXGI_FORMAT_BC2_UNORM: return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC2_UNORM_SRGB: return VK_FORMAT_BC2_SRGB_BLOCK;
            case DXGI_FORMAT_BC3_UNORM: return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM_SRGB: return VK_FORMAT_BC3_SRGB_BLOCK;
            case DXGI_FORMAT_BC4_UNORM: return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_SNORM: return VK_FORMAT_BC4_SNORM_BLOCK;
            case DXGI_FORMAT_BC5_UNORM: return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_SNORM: return VK_FORMAT_BC5_SNORM_BLOCK;
            case DXGI_FORMAT_BC7_UNORM: return VK_FORMAT_BC7_UNORM_BLOCK;
            case DXGI_FORMAT_BC7_UNORM_SRGB: return VK_FORMAT_BC7_SRGB_BLOCK;
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

        uint32_t GetFormatStride(Render::Format format)
        {
            switch (format)
            {
            case Render::Format::R32G32B32A32_FLOAT: return 16;
            case Render::Format::R16G16B16A16_FLOAT: return 8;
            case Render::Format::R32G32B32_FLOAT: return 12;
            case Render::Format::R32G32_FLOAT: return 8;
            case Render::Format::R16G16_FLOAT: return 4;
            case Render::Format::R32_FLOAT: return 4;
            case Render::Format::R16_FLOAT: return 2;

            case Render::Format::R32G32B32A32_UINT: return 16;
            case Render::Format::R16G16B16A16_UINT: return 8;
            case Render::Format::R8G8B8A8_UINT: return 4;
            case Render::Format::R32G32B32_UINT: return 12;
            case Render::Format::R32G32_UINT: return 8;
            case Render::Format::R16G16_UINT: return 4;
            case Render::Format::R8G8_UINT: return 2;

            case Render::Format::R32G32B32A32_INT: return 16;
            case Render::Format::R16G16B16A16_INT: return 8;
            case Render::Format::R8G8B8A8_INT: return 4;
            case Render::Format::R32G32B32_INT: return 12;
            case Render::Format::R32G32_INT: return 8;
            case Render::Format::R16G16_INT: return 4;
            case Render::Format::R8G8_INT: return 2;

            case Render::Format::R8G8B8A8_UNORM: return 4;
            case Render::Format::R8G8B8A8_UNORM_SRGB: return 4;
            case Render::Format::R16G16_UNORM: return 4;
            case Render::Format::R8G8_UNORM: return 2;
            case Render::Format::R16_UNORM: return 2;
            case Render::Format::R8_UNORM: return 1;

            case Render::Format::R16G16_NORM: return 4;
            case Render::Format::R8G8_NORM: return 2;
            case Render::Format::R16_NORM: return 2;
            case Render::Format::R8_NORM: return 1;

            case Render::Format::R16_UINT: return 2;
            case Render::Format::R32_UINT: return 4;
            default:
                return 0;
            }
        }

        VkPrimitiveTopology GetVkPrimitiveTopology(Render::PrimitiveType primitiveType)
        {
            switch (primitiveType)
            {
            case Render::PrimitiveType::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case Render::PrimitiveType::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case Render::PrimitiveType::LineStrip: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case Render::PrimitiveType::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case Render::PrimitiveType::TriangleList:
            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            }
        }

        const std::vector<const char*> validationLayers =
        {
            "VK_LAYER_KHRONOS_validation",
        };

        const std::vector<const char*> deviceExtensions =
        {
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

        static constexpr std::string_view SemanticNameList[] =
        {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMAL",
            "NORMAL",
            "COLOR",
        };

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

            TYPE * const * const get(void) const
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
                : DescribedVideoObject<Render::SamplerState>(description)
                , device(device)
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

            InputLayout(const std::vector<Render::InputElement> &elementList)
                : elementList(elementList)
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
            : public Render::Buffer
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
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
                : Resource()
                , ShaderResourceView()  
                , UnorderedAccessView()
                , description(description)
                , device(device)
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
            : virtual public Render::Texture
            , public BaseTexture
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
            : public Texture
            , public Resource
            , public ShaderResourceView
        {
        public:
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;

            ViewTexture(const Render::Texture::Description &description)
                : Texture(description)
                , Resource()
                , ShaderResourceView()
            {
            }

            virtual ~ViewTexture(void)
            {
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
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            UnorderedViewTexture(const Render::Texture::Description &description)
                : Texture(description)
                , Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }
        };

        class Target
            : virtual public Render::Target
            , public BaseTexture
        {
        public:
            Render::ViewPort viewPort;

        public:
            Target(const Render::Texture::Description &description)
                : BaseTexture(description)
                , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(description.width), float(description.height)), 0.0f, 1.0f)
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
            : public Target
            , public Resource
            , public RenderTargetView
        {
        public:
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView imageView = VK_NULL_HANDLE;
            VkSampler sampler = VK_NULL_HANDLE;
            VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            bool supportsMipBlit = false;

            TargetTexture(const Render::Texture::Description &description)
                : Target(description)
                , Resource()
                , RenderTargetView()
            {
            }

            virtual ~TargetTexture(void)
            {
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
            : virtual public TargetTexture
            , public ShaderResourceView
        {
        public:
            TargetViewTexture(const Render::Texture::Description &description)
                : TargetTexture(description)
                , ShaderResourceView()
            {
            }

            virtual ~TargetViewTexture(void) = default;
        };

        class UnorderedTargetViewTexture
            : virtual public TargetTexture
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            UnorderedTargetViewTexture(const Render::Texture::Description &description)
                : TargetTexture(description)
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }

            virtual ~UnorderedTargetViewTexture(void) = default;
        };

        class DepthTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
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
                : Texture(description)
                , Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }

            virtual ~DepthTexture(void)
            {
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
                : information(information)
                , device(device)
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

            void enqueueGenerateMipMapsCommand(Context *sourceContext, Render::Texture *texture);
            void enqueueComputeDispatchCommand(Context *sourceContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
            void enqueueClearRenderTargetCommand(Context *sourceContext, Render::Target *renderTarget, Math::Float4 const &clearColor);
            void enqueueClearDepthStencilCommand(Context *sourceContext, Render::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil);
            void enqueueCopyResourceCommand(Context *sourceContext, Render::Object *destination, Render::Object *source);

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
                        context->currentVertexProgram = getObject<VertexProgram>(program);
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
                        context->currentPixelProgram = getObject<PixelProgram>(program);
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
                VkRect2D currentScissor = { {0, 0}, {0, 0} };
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
                    : pipelineDevice(pipelineDevice)
                    , isDeferredContext(isDeferredContext)
                    , computeSystemHandler(new ComputePipeline(this))
                    , vertexSystemHandler(new VertexPipeline(this))
                    , geomtrySystemHandler(new GeometryPipeline(this))
                    , pixelSystemHandler(new PixelPipeline(this))
                {
                    assert(computeSystemHandler);
                    assert(vertexSystemHandler);
                    assert(geomtrySystemHandler);
                    assert(pixelSystemHandler);
                }

                // Render::Context
                Pipeline * const computePipeline(void)
                {
                    assert(computeSystemHandler);

                    return computeSystemHandler.get();
                }

                Pipeline * const vertexPipeline(void)
                {
                    assert(vertexSystemHandler);

                    return vertexSystemHandler.get();
                }

                Pipeline * const geometryPipeline(void)
                {
                    assert(geomtrySystemHandler);

                    return geomtrySystemHandler.get();
                }

                Pipeline * const pixelPipeline(void)
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
                    currentScissor = { {0, 0}, {0, 0} };
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
                        pipelineDevice->pendingClearColor =
                        {
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
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
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

                    for (uint32_t targetIndex = 0; targetIndex < currentRenderTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    if (isDeferredContext)
                    {
                        pipelineDevice->deferredContextDrawCommands[this].push_back(command);
                    }
                    else
                    {
                        pipelineDevice->pendingDrawCommands.push_back(command);
                    }
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
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
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
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

                    for (uint32_t targetIndex = 0; targetIndex < currentRenderTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    if (isDeferredContext)
                    {
                        pipelineDevice->deferredContextDrawCommands[this].push_back(command);
                    }
                    else
                    {
                        pipelineDevice->pendingDrawCommands.push_back(command);
                    }
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
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
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
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

                    for (uint32_t targetIndex = 0; targetIndex < currentRenderTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    if (isDeferredContext)
                    {
                        pipelineDevice->deferredContextDrawCommands[this].push_back(command);
                    }
                    else
                    {
                        pipelineDevice->pendingDrawCommands.push_back(command);
                    }
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!pipelineDevice)
                    {
                        return;
                    }

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
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
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    command.renderState = currentRenderState;

                    std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
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

                    for (uint32_t targetIndex = 0; targetIndex < currentRenderTargetCount; ++targetIndex)
                    {
                        auto *target = currentRenderTargetList[targetIndex];
                        if (!target)
                        {
                            continue;
                        }

                        command.offscreenImages[targetIndex] = target->image;
                        command.offscreenImageViews[targetIndex] = target->imageView;
                        command.offscreenFormats[targetIndex] = GetVkFormat(target->getDescription().format);
                        command.offscreenExtents[targetIndex].width = std::max(target->getDescription().width, 1u);
                        command.offscreenExtents[targetIndex].height = std::max(target->getDescription().height, 1u);
                        pipelineDevice->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                    }

                    if (isDeferredContext)
                    {
                        pipelineDevice->deferredContextDrawCommands[this].push_back(command);
                    }
                    else
                    {
                        pipelineDevice->pendingDrawCommands.push_back(command);
                    }
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
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
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
            Window::Device * window = nullptr;
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
                VkRect2D scissor = {{ 0, 0 }, { 1, 1 }};
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
                VkClearColorValue clearRenderTargetColor = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

                DepthTexture *clearDepthTarget = nullptr;
                uint32_t clearDepthStencilFlags = 0;
                float clearDepthValue = 1.0f;
                uint32_t clearStencilValue = 0;
            };

            std::vector<DrawCommand> pendingDrawCommands;
            std::map<Context *, std::vector<DrawCommand>> deferredContextDrawCommands;
            std::map<uint64_t, std::vector<DrawCommand>> deferredCommandLists;
            Render::BufferVersioningPolicy constantBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            Render::BufferVersioningPolicy vertexBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            Render::BufferVersioningPolicy indexBufferVersioningPolicy = { Render::BufferVersioningMode::FixedRing, static_cast<uint8_t>(Buffer::VersionSlotCount) };
            uint64_t nextCommandListIdentifier = 1;
            std::set<Buffer *> versionedConstantBuffersInFlight;
            std::map<VkImage, VkImageLayout> offscreenImageLayouts;
            std::map<VkImageView, std::pair<VkImage, VkExtent2D>> persistentImageViewLookup;
            std::mutex persistentImageViewLookupMutex;
            uint64_t presentFrameIndex = 0;
            VkViewport currentViewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
            bool deviceLost = false;
            bool loggedDeviceLost = false;
            bool samplerAnisotropySupported = false;
            float maxSamplerAnisotropy = 1.0f;

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

                bool operator < (const PipelineKey &other) const
                {
                    if (vertexModule != other.vertexModule) return vertexModule < other.vertexModule;
                    if (pixelModule != other.pixelModule) return pixelModule < other.pixelModule;
                    if (inputLayout != other.inputLayout) return inputLayout < other.inputLayout;
                    if (renderPass != other.renderPass) return renderPass < other.renderPass;
                    if (primitiveType != other.primitiveType) return primitiveType < other.primitiveType;
                    if (blendEnabled != other.blendEnabled) return blendEnabled < other.blendEnabled;
                    if (depthEnabled != other.depthEnabled) return depthEnabled < other.depthEnabled;
                    if (depthWrite != other.depthWrite) return depthWrite < other.depthWrite;
                    if (depthCompareFunction != other.depthCompareFunction) return depthCompareFunction < other.depthCompareFunction;
                    if (cullMode != other.cullMode) return cullMode < other.cullMode;
                    return frontCounterClockwise < other.frontCounterClockwise;
                }
            };

            struct FramebufferKey
            {
                VkRenderPass renderPass = VK_NULL_HANDLE;
                VkExtent2D extent = { 0, 0 };
                uint32_t attachmentCount = 0;
                std::array<VkImageView, 9> attachments{};

                bool operator < (const FramebufferKey &other) const
                {
                    if (renderPass != other.renderPass) return renderPass < other.renderPass;
                    if (extent.width != other.extent.width) return extent.width < other.extent.width;
                    if (extent.height != other.extent.height) return extent.height < other.extent.height;
                    if (attachmentCount != other.attachmentCount) return attachmentCount < other.attachmentCount;
                    return attachments < other.attachments;
                }
            };

            std::map<PipelineKey, VkPipeline> graphicsPipelineCache;
            std::set<PipelineKey> failedGraphicsPipelineKeys;
            std::map<VkShaderModule, VkPipeline> computePipelineCache;
            std::set<VkShaderModule> failedComputePipelineModules;
            std::map<FramebufferKey, VkFramebuffer> offscreenFramebufferCache;

            VkClearColorValue pendingClearColor = {{ 0.1f, 0.1f, 0.15f, 1.0f }};

            Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;

        private:
            static std::mutex &getUploadCommandPoolMutex(void)
            {
                static std::mutex *mutex = new std::mutex();
                return *mutex;
            }

            static std::mutex &getQueueSubmitMutex(void)
            {
                static std::mutex *mutex = new std::mutex();
                return *mutex;
            }

            static std::mutex &getDrawCommandMutex(void)
            {
                static std::mutex *mutex = new std::mutex();
                return *mutex;
            }

            static std::mutex &getTextureDecodeMutex(void)
            {
                static std::mutex *mutex = new std::mutex();
                return *mutex;
            }

            Render::BufferVersioningPolicy normalizeBufferVersioningPolicy(Render::BufferVersioningPolicy policy) const
            {
                const uint32_t clampedRingSize = std::min<uint32_t>(policy.ringSize, Buffer::VersionSlotCount);
                policy.ringSize = static_cast<uint8_t>(clampedRingSize);
                if ((policy.mode != Render::BufferVersioningMode::FixedRing) || (clampedRingSize < 2))
                {
                    policy.mode = Render::BufferVersioningMode::Disabled;
                }

                return policy;
            }

            void applyBufferVersioningPolicyOverrides(Render::Device::Description const &description)
            {
                constantBufferVersioningPolicy = normalizeBufferVersioningPolicy(description.constantBufferVersioningPolicy);
                vertexBufferVersioningPolicy = normalizeBufferVersioningPolicy(description.vertexBufferVersioningPolicy);
                indexBufferVersioningPolicy = normalizeBufferVersioningPolicy(description.indexBufferVersioningPolicy);

                getContext()->log(
                    Gek::Context::Info,
                    "Vulkan buffer versioning policy: cb={}({}), vb={}({}), ib={}({})",
                    static_cast<uint32_t>(constantBufferVersioningPolicy.ringSize),
                    (constantBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? "FixedRing" : "Disabled",
                    static_cast<uint32_t>(vertexBufferVersioningPolicy.ringSize),
                    (vertexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? "FixedRing" : "Disabled",
                    static_cast<uint32_t>(indexBufferVersioningPolicy.ringSize),
                    (indexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? "FixedRing" : "Disabled");
            }

            bool shouldUseVersionedWriteDiscardBuffer(Render::Buffer::Description const &description) const
            {
                if ((description.flags & Render::Buffer::Flags::Mappable) == 0)
                {
                    return false;
                }

                if (description.type == Render::Buffer::Type::Constant)
                {
                    return (constantBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) &&
                           (constantBufferVersioningPolicy.ringSize >= 2);
                }

                if (description.type == Render::Buffer::Type::Vertex || description.type == Render::Buffer::Type::Index)
                {
                      if (description.type == Render::Buffer::Type::Vertex)
                      {
                       return (vertexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) &&
                           (vertexBufferVersioningPolicy.ringSize >= 2);
                      }

                      return (indexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) &&
                          (indexBufferVersioningPolicy.ringSize >= 2);
                }

                return false;
            }

            uint32_t getVersionedRingSize(Render::Buffer::Description const &description) const
            {
                if (description.type == Render::Buffer::Type::Constant)
                {
                    return std::max<uint32_t>(2, std::min<uint32_t>(constantBufferVersioningPolicy.ringSize, Buffer::VersionSlotCount));
                }

                if (description.type == Render::Buffer::Type::Vertex)
                {
                    return std::max<uint32_t>(2, std::min<uint32_t>(vertexBufferVersioningPolicy.ringSize, Buffer::VersionSlotCount));
                }

                if (description.type == Render::Buffer::Type::Index)
                {
                    return std::max<uint32_t>(2, std::min<uint32_t>(indexBufferVersioningPolicy.ringSize, Buffer::VersionSlotCount));
                }

                return Buffer::VersionSlotCount;
            }

            void releaseVersionedConstantBufferSlots(void)
            {
                for (auto *buffer : versionedConstantBuffersInFlight)
                {
                    if (!buffer || !buffer->usesVersionedConstantBacking)
                    {
                        continue;
                    }

                    buffer->versionInUseList.fill(false);
                }

                versionedConstantBuffersInFlight.clear();
            }

            uint8_t captureConstantBufferVersion(Buffer *buffer)
            {
                if (!buffer || !buffer->usesVersionedConstantBacking)
                {
                    return 0;
                }

                const uint8_t versionIndex = static_cast<uint8_t>(std::min<uint32_t>(buffer->activeVersionIndex, Buffer::VersionSlotCount - 1));
                buffer->versionInUseList[versionIndex] = true;
                versionedConstantBuffersInFlight.insert(buffer);
                return versionIndex;
            }

            uint8_t captureVersionedBufferSlot(Buffer *buffer)
            {
                return captureConstantBufferVersion(buffer);
            }

            VkBuffer getCapturedConstantVkBuffer(Buffer const *buffer, uint8_t versionIndex) const
            {
                if (!buffer)
                {
                    return VK_NULL_HANDLE;
                }

                if (!buffer->usesVersionedConstantBacking)
                {
                    return buffer->buffer;
                }

                if (versionIndex >= Buffer::VersionSlotCount)
                {
                    return VK_NULL_HANDLE;
                }

                return buffer->versionBufferList[versionIndex];
            }

            VkBuffer getCapturedVkBuffer(Buffer const *buffer, uint8_t versionIndex) const
            {
                return getCapturedConstantVkBuffer(buffer, versionIndex);
            }

            uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
            {
                VkPhysicalDeviceMemoryProperties memoryProperties{};
                vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

                for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
                {
                    if ((typeFilter & (1u << index)) &&
                        (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties)
                    {
                        return index;
                    }
                }

                return UINT32_MAX;
            }

            Buffer *createBufferSnapshot(Buffer *sourceBuffer)
            {
                if (!sourceBuffer || sourceBuffer->buffer == VK_NULL_HANDLE)
                {
                    return nullptr;
                }

                if (sourceBuffer->usesVersionedConstantBacking)
                {
                    return sourceBuffer;
                }

                Render::BufferPtr snapshotResource = createBuffer(sourceBuffer->getDescription(), nullptr);
                Buffer *snapshotBuffer = getObject<Buffer>(snapshotResource.get());
                if (!snapshotBuffer || snapshotBuffer->buffer == VK_NULL_HANDLE)
                {
                    return sourceBuffer;
                }

                void *sourceData = sourceBuffer->mappedData;
                bool sourceMappedForCopy = false;
                if (!sourceData)
                {
                    if (vkMapMemory(device, sourceBuffer->memory, 0, sourceBuffer->size, 0, &sourceData) != VK_SUCCESS)
                    {
                        return sourceBuffer;
                    }

                    sourceMappedForCopy = true;
                }

                void *destinationData = snapshotBuffer->mappedData;
                bool destinationMappedForCopy = false;
                if (!destinationData)
                {
                    if (vkMapMemory(device, snapshotBuffer->memory, 0, snapshotBuffer->size, 0, &destinationData) != VK_SUCCESS)
                    {
                        if (sourceMappedForCopy)
                        {
                            vkUnmapMemory(device, sourceBuffer->memory);
                        }

                        return sourceBuffer;
                    }

                    destinationMappedForCopy = true;
                }

                const VkDeviceSize copySize = std::min(sourceBuffer->size, snapshotBuffer->size);
                std::memcpy(destinationData, sourceData, static_cast<size_t>(copySize));

                if (destinationMappedForCopy)
                {
                    vkUnmapMemory(device, snapshotBuffer->memory);
                }

                if (sourceMappedForCopy)
                {
                    vkUnmapMemory(device, sourceBuffer->memory);
                }

                return snapshotBuffer;
            }

            Buffer *captureBufferSnapshot(Buffer *buffer, bool shouldSnapshot)
            {
                if (!buffer)
                {
                    return nullptr;
                }

                const bool bufferIsMappable = ((buffer->getDescription().flags & Render::Buffer::Flags::Mappable) != 0);
                if (!shouldSnapshot && !bufferIsMappable)
                {
                    return buffer;
                }

                return createBufferSnapshot(buffer);
            }

            bool checkInstanceExtensionSupport(std::vector<const char*> &instanceExtensions)
            {
                uint32_t extensionCount = 0;
                vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

                std::set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());
                for (const auto& extension : availableExtensions)
                {
                    requiredExtensions.erase(extension.extensionName);
                }

                return requiredExtensions.empty();
            }

            void createInstance(void)
            {
                VkApplicationInfo appInfo{};
                appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
                appInfo.pApplicationName = "GEK Engine";
                appInfo.applicationVersion = VK_MAKE_VERSION(10, 0, 0);
                appInfo.pEngineName = "GEK Engine";
                appInfo.engineVersion = VK_MAKE_VERSION(10, 0, 0);
                appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

                std::vector<const char*> instanceExtensions = 
                {
                    VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef WIN32
                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
                    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
                    
                };

                if (!checkInstanceExtensionSupport(instanceExtensions))
                {
                    throw std::runtime_error("Required instances extensions not available");
                }

                VkInstanceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                createInfo.pApplicationInfo = &appInfo;
                createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
                createInfo.ppEnabledExtensionNames = instanceExtensions.data();
                if (enableValidationLayer)
                {
                    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                    createInfo.ppEnabledLayerNames = validationLayers.data();
                }
                else
                {
                    createInfo.enabledLayerCount = 0;
                }

                VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
                if (result != VK_SUCCESS)
                {
                    throw std::runtime_error("Unable to create rendering device and context");
                }

                getContext()->log(Gek::Context::Info, "Vulkan Instance Created");
            }

            bool checkValidationLayerSupport(void)
            {
                uint32_t layerCount;
                vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

                std::vector<VkLayerProperties> availableLayers(layerCount);
                vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

                for (const char* layerName : validationLayers)
                {
                    bool layerFound = false;
                    for (const auto& layerProperties : availableLayers)
                    {
                        if (strcmp(layerName, layerProperties.layerName) == 0)
                        {
                            layerFound = true;
                            break;
                        }
                    }

                    if (!layerFound)
                    {
                        return false;
                    }
                }

                return true;
            }

            bool checkDeviceExtensionSupport(VkPhysicalDevice checkDevice)
            {
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(checkDevice, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(checkDevice, nullptr, &extensionCount, availableExtensions.data());

                std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
                for (const auto& extension : availableExtensions)
                {
                    requiredExtensions.erase(extension.extensionName);
                }

                return requiredExtensions.empty();
            }

            uint32_t rateDeviceSuitability(VkPhysicalDevice checkDevice)
            {
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(checkDevice, &deviceFeatures);
                if (!deviceFeatures.geometryShader)
                {
                    return 0;
                }

                if (!checkDeviceExtensionSupport(checkDevice))
                {
                    return 0;
                  }

                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(checkDevice);
                if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
                {
                    return 0;
                }

                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(checkDevice, &deviceProperties);

                uint32_t score = 0;
                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    score += 1000;
                }

                score += deviceProperties.limits.maxImageDimension2D;
                return score;
            }

            void pickPhysicalDevice(void)
            {
                uint32_t deviceCount = 0;
                vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
                if (deviceCount == 0)
                {
                    throw std::runtime_error("failed to find GPUs with Vulkan support!");
                }

                std::vector<VkPhysicalDevice> availableDevices(deviceCount);
                vkEnumeratePhysicalDevices(instance, &deviceCount, availableDevices.data());

                std::multimap<uint32_t, VkPhysicalDevice> candidates;
                for (const auto& device : availableDevices)
                {
                    uint32_t score = rateDeviceSuitability(device);
                    candidates.insert(std::make_pair(score, device));
                }

                if (!candidates.empty() && candidates.rbegin()->first > 0)
                {
                    physicalDevice = candidates.rbegin()->second;
                }
                else
                {
                    throw std::runtime_error("failed to find a suitable GPU!");
                }

                getContext()->log(Gek::Context::Info, "Found suitable Vulkan physical device");
            }

            QueueFamilyIndices findQueueFamilies(void)
            {
                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

                std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

                int familyIndex = 0;
                QueueFamilyIndices indices;
                for (const auto& queueFamily : queueFamilies)
                {
                    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    {
                        indices.graphicsFamily = familyIndex;
                    }

                    VkBool32 presentSupport = false;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupport);

                    if (presentSupport)
                    {
                        indices.presentFamily = familyIndex;
                    }

                    if (indices.isComplete())
                    {
                        break;
                    }

                    familyIndex++;
                }

                return indices;
            }

            void createLogicalDevice(void)
            {
                QueueFamilyIndices indices = findQueueFamilies();

                float queuePriority = 1.0f;
                std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
                std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
                for (uint32_t queueFamily : uniqueQueueFamilies)
                {
                    VkDeviceQueueCreateInfo queueCreateInfo{};
                    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                    queueCreateInfo.queueFamilyIndex = queueFamily;
                    queueCreateInfo.queueCount = 1;
                    queueCreateInfo.pQueuePriorities = &queuePriority;
                    queueCreateInfos.push_back(queueCreateInfo);
                }

                VkPhysicalDeviceFeatures deviceFeatures{};
                VkPhysicalDeviceFeatures availableDeviceFeatures{};
                vkGetPhysicalDeviceFeatures(physicalDevice, &availableDeviceFeatures);

                VkPhysicalDeviceProperties deviceProperties{};
                vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
                maxSamplerAnisotropy = std::max(1.0f, deviceProperties.limits.maxSamplerAnisotropy);
                samplerAnisotropySupported = (availableDeviceFeatures.samplerAnisotropy == VK_TRUE);

                if (!availableDeviceFeatures.shaderStorageImageReadWithoutFormat || !availableDeviceFeatures.shaderStorageImageWriteWithoutFormat)
                {
                    throw std::runtime_error("Vulkan device missing required formatless storage image read/write features");
                }

                deviceFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;
                deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;
                if (samplerAnisotropySupported)
                {
                    deviceFeatures.samplerAnisotropy = VK_TRUE;
                }

                VkPhysicalDeviceVulkan11Features availableVulkan11Features{};
                availableVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

                VkPhysicalDeviceFeatures2 availableFeatures2{};
                availableFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                availableFeatures2.pNext = &availableVulkan11Features;
                vkGetPhysicalDeviceFeatures2(physicalDevice, &availableFeatures2);

                if (!availableVulkan11Features.shaderDrawParameters)
                {
                    throw std::runtime_error("Vulkan device missing required shaderDrawParameters feature for SPIR-V DrawParameters capability");
                }

                VkPhysicalDeviceVulkan11Features enabledVulkan11Features{};
                enabledVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
                enabledVulkan11Features.shaderDrawParameters = VK_TRUE;

                VkDeviceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
                createInfo.pQueueCreateInfos = queueCreateInfos.data();
                createInfo.pEnabledFeatures = &deviceFeatures;
                createInfo.pNext = &enabledVulkan11Features;
                createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
                createInfo.ppEnabledExtensionNames = deviceExtensions.data();
                if (enableValidationLayer)
                {
                    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                    createInfo.ppEnabledLayerNames = validationLayers.data();
                }
                else
                {
                    createInfo.enabledLayerCount = 0;
                }

                if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create logical device!");
                }

                vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
                vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
                getContext()->log(Gek::Context::Info, "Vulkan logical device created");
            }

            void createSurface(void)
            {
#ifdef WIN32
                VkWin32SurfaceCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                createInfo.hinstance = GetModuleHandle(nullptr);
                createInfo.hwnd = reinterpret_cast<HWND>(window->getWindowData(0));
                auto result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
#else
                VkXlibSurfaceCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                createInfo.dpy = reinterpret_cast<Display *>(window->getWindowData(0));
                createInfo.window = *reinterpret_cast<::Window *>(window->getWindowData(1));
                auto result = vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface);
#endif
                if (result == VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Info, "Vulkan surface created");
                }
                else
                {
                    throw std::runtime_error("failed to create window surface!");
                }
            }

            struct SwapChainSupportDetails
            {
                VkSurfaceCapabilitiesKHR capabilities;
                std::vector<VkSurfaceFormatKHR> formats;
                std::vector<VkPresentModeKHR> presentModes;
            };

            SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
            {
                SwapChainSupportDetails details;
                vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

                uint32_t formatCount;
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
                if (formatCount != 0)
                {
                    details.formats.resize(formatCount);
                    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
                }

                uint32_t presentModeCount;
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
                if (presentModeCount != 0)
                {
                    details.presentModes.resize(presentModeCount);
                    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
                }

                return details;
            }

            VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        return availableFormat;
                    }
                }

                return availableFormats[0];
            }

            VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
            {
                for (const auto& availablePresentMode : availablePresentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
                    {
                        return availablePresentMode;
                    }
                }

                return VK_PRESENT_MODE_FIFO_KHR;
            }

            VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
            {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                {
                    return capabilities.currentExtent;
                }
                else
                {
                    auto clientRectangle = window->getClientRectangle();
                    VkExtent2D actualExtent =
                    {
                        static_cast<uint32_t>(clientRectangle.size.x),
                        static_cast<uint32_t>(clientRectangle.size.y),
                    };

                    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
                    return actualExtent;
                }
            }

            void createSwapChain(void)
            {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

                VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
                VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
                VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

                uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
                if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
                {
                    imageCount = swapChainSupport.capabilities.maxImageCount;
                }

                QueueFamilyIndices indices = findQueueFamilies();
                uint32_t queueFamilyIndices[] = 
                {
                    indices.graphicsFamily.value(),
                    indices.presentFamily.value()
                };

                VkSwapchainCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                createInfo.surface = surface;
                createInfo.minImageCount = imageCount;
                createInfo.imageFormat = surfaceFormat.format;
                createInfo.imageColorSpace = surfaceFormat.colorSpace;
                createInfo.imageExtent = extent;
                createInfo.imageArrayLayers = 1;
                createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                if ((swapChainSupport.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0)
                {
                    createInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                }
                if (indices.graphicsFamily != indices.presentFamily)
                {
                    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = 2;
                    createInfo.pQueueFamilyIndices = queueFamilyIndices;
                }
                else
                {
                    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                }

                createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
                createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                createInfo.presentMode = presentMode;
                createInfo.clipped = VK_TRUE;
                createInfo.oldSwapchain = VK_NULL_HANDLE;
                if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create swap chain!");
                }

                vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
                swapChainImages.resize(imageCount);
                vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
                swapChainImageLayouts.assign(swapChainImages.size(), VK_IMAGE_LAYOUT_UNDEFINED);
                swapChainImageFormat = surfaceFormat.format;
                swapChainExtent = extent;

                getContext()->log(Gek::Context::Info, "Vulkan swap chain created");
            }

            void createImageViews(void)
            {
                swapChainImageViews.resize(swapChainImages.size());
                for (size_t i = 0; i < swapChainImages.size(); i++)
                {
                    VkImageViewCreateInfo createInfo{};
                    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                    createInfo.image = swapChainImages[i];

                    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                    createInfo.format = swapChainImageFormat;

                    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

                    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    createInfo.subresourceRange.baseMipLevel = 0;
                    createInfo.subresourceRange.levelCount = 1;
                    createInfo.subresourceRange.baseArrayLayer = 0;
                    createInfo.subresourceRange.layerCount = 1;
                    if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create image views!");
                    }
                }

                getContext()->log(Gek::Context::Info, "Vulkan image views created");
            }

            void destroyRenderPassResources(void)
            {
                for (auto pipelinePair : graphicsPipelineCache)
                {
                    if (pipelinePair.second != VK_NULL_HANDLE)
                    {
                        vkDestroyPipeline(device, pipelinePair.second, nullptr);
                    }
                }
                graphicsPipelineCache.clear();
                failedGraphicsPipelineKeys.clear();

                for (auto pipelinePair : computePipelineCache)
                {
                    if (pipelinePair.second != VK_NULL_HANDLE)
                    {
                        vkDestroyPipeline(device, pipelinePair.second, nullptr);
                    }
                }
                computePipelineCache.clear();
                failedComputePipelineModules.clear();

                for (auto framebuffer : swapChainFramebuffers)
                {
                    vkDestroyFramebuffer(device, framebuffer, nullptr);
                }
                swapChainFramebuffers.clear();

                for (auto &framebufferPair : offscreenFramebufferCache)
                {
                    if (framebufferPair.second != VK_NULL_HANDLE)
                    {
                        vkDestroyFramebuffer(device, framebufferPair.second, nullptr);
                    }
                }
                offscreenFramebufferCache.clear();

                if (depthImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(device, depthImageView, nullptr);
                    depthImageView = VK_NULL_HANDLE;
                }

                if (depthImage != VK_NULL_HANDLE)
                {
                    vkDestroyImage(device, depthImage, nullptr);
                    depthImage = VK_NULL_HANDLE;
                }

                if (depthMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(device, depthMemory, nullptr);
                    depthMemory = VK_NULL_HANDLE;
                }

                if (renderPass != VK_NULL_HANDLE)
                {
                    vkDestroyRenderPass(device, renderPass, nullptr);
                    renderPass = VK_NULL_HANDLE;
                }
            }

            void createDepthImage(void)
            {
                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = swapChainExtent.width;
                imageInfo.extent.height = swapChainExtent.height;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = depthFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create depth image");
                }

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(device, depthImage, &memRequirements);

                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

                uint32_t memType = 0;
                for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
                {
                    if ((memRequirements.memoryTypeBits & (1 << i)) &&
                        (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                    {
                        memType = i;
                        break;
                    }
                }

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = memType;

                if (vkAllocateMemory(device, &allocInfo, nullptr, &depthMemory) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to allocate depth image memory");
                }

                vkBindImageMemory(device, depthImage, depthMemory, 0);

                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = depthImage;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = depthFormat;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create depth image view");
                }

                getContext()->log(Gek::Context::Info, "Vulkan depth image created");
            }

            void createRenderPassResources(void)
            {
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format = swapChainImageFormat;
                colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = depthFormat;
                depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkAttachmentReference depthAttachmentRef{};
                depthAttachmentRef.attachment = 1;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;
                subpass.pDepthStencilAttachment = &depthAttachmentRef;

                VkSubpassDependency dependency{};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = 2;
                renderPassInfo.pAttachments = attachments;
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = 1;
                renderPassInfo.pDependencies = &dependency;
                if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create Vulkan render pass");
                }

                swapChainFramebuffers.resize(swapChainImageViews.size());
                for (size_t imageIndex = 0; imageIndex < swapChainImageViews.size(); ++imageIndex)
                {
                    VkImageView attachmentViews[2] = { swapChainImageViews[imageIndex], depthImageView };
                    VkFramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferInfo.renderPass = renderPass;
                    framebufferInfo.attachmentCount = 2;
                    framebufferInfo.pAttachments = attachmentViews;
                    framebufferInfo.width = swapChainExtent.width;
                    framebufferInfo.height = swapChainExtent.height;
                    framebufferInfo.layers = 1;

                    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[imageIndex]) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create Vulkan framebuffer");
                    }
                }
            }

            VkRenderPass getOrCreateOffscreenRenderPass(const std::vector<VkFormat> &formats, VkFormat depthAttachmentFormat)
            {
                if (formats.empty())
                {
                    return VK_NULL_HANDLE;
                }

                const bool withDepth = (depthAttachmentFormat != VK_FORMAT_UNDEFINED);
                auto cacheKey = std::make_pair(formats, depthAttachmentFormat);
                auto renderPassSearch = offscreenRenderPassCache.find(cacheKey);
                if (renderPassSearch != std::end(offscreenRenderPassCache))
                {
                    return renderPassSearch->second;
                }

                std::vector<VkAttachmentDescription> colorAttachments;
                colorAttachments.reserve(formats.size());

                std::vector<VkAttachmentReference> colorAttachmentRefs;
                colorAttachmentRefs.reserve(formats.size());

                for (uint32_t attachmentIndex = 0; attachmentIndex < formats.size(); ++attachmentIndex)
                {
                    VkAttachmentDescription colorAttachment{};
                    colorAttachment.format = formats[attachmentIndex];
                    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    colorAttachments.push_back(colorAttachment);

                    VkAttachmentReference colorAttachmentRef{};
                    colorAttachmentRef.attachment = attachmentIndex;
                    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    colorAttachmentRefs.push_back(colorAttachmentRef);
                }

                VkAttachmentDescription depthAttachment{};
                VkAttachmentReference depthAttachmentRef{};
                if (withDepth)
                {
                    depthAttachment.format = depthAttachmentFormat;
                    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

                    depthAttachmentRef.attachment = static_cast<uint32_t>(colorAttachments.size());
                    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
                subpass.pColorAttachments = colorAttachmentRefs.data();
                if (withDepth)
                {
                    subpass.pDepthStencilAttachment = &depthAttachmentRef;
                }

                VkSubpassDependency dependency{};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                if (withDepth)
                {
                    dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                }

                std::vector<VkAttachmentDescription> allAttachments = colorAttachments;
                if (withDepth)
                {
                    allAttachments.push_back(depthAttachment);
                }

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = static_cast<uint32_t>(allAttachments.size());
                renderPassInfo.pAttachments = allAttachments.data();
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = 1;
                renderPassInfo.pDependencies = &dependency;

                VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
                if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenRenderPass) != VK_SUCCESS)
                {
                    return VK_NULL_HANDLE;
                }

                offscreenRenderPassCache[cacheKey] = offscreenRenderPass;
                return offscreenRenderPass;
            }

            void createDescriptorResources(void)
            {
                std::vector<VkDescriptorSetLayoutBinding> bindings;
                bindings.reserve((PixelResourceSlotCount * 6));

                for (uint32_t slot = 0; slot < PixelResourceSlotCount; ++slot)
                {
                    VkDescriptorSetLayoutBinding sampledImageBinding{};
                    sampledImageBinding.binding = DescriptorSampledImageBase + slot;
                    sampledImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                    sampledImageBinding.descriptorCount = 1;
                    sampledImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
                    bindings.push_back(sampledImageBinding);

                    VkDescriptorSetLayoutBinding storageBufferBinding{};
                    storageBufferBinding.binding = DescriptorStorageBufferBase + slot;
                    storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    storageBufferBinding.descriptorCount = 1;
                    storageBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
                    bindings.push_back(storageBufferBinding);

                    VkDescriptorSetLayoutBinding vertexUniformBinding{};
                    vertexUniformBinding.binding = DescriptorVertexUniformBufferBase + slot;
                    vertexUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    vertexUniformBinding.descriptorCount = 1;
                    vertexUniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
                    bindings.push_back(vertexUniformBinding);

                    VkDescriptorSetLayoutBinding pixelUniformBinding{};
                    pixelUniformBinding.binding = DescriptorPixelUniformBufferBase + slot;
                    pixelUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    pixelUniformBinding.descriptorCount = 1;
                    pixelUniformBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(pixelUniformBinding);

                    VkDescriptorSetLayoutBinding samplerBinding{};
                    samplerBinding.binding = DescriptorSamplerBase + slot;
                    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                    samplerBinding.descriptorCount = 1;
                    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
                    bindings.push_back(samplerBinding);

                    VkDescriptorSetLayoutBinding storageImageBinding{};
                    storageImageBinding.binding = DescriptorStorageImageBase + slot;
                    storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    storageImageBinding.descriptorCount = 1;
                    storageImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
                    bindings.push_back(storageImageBinding);
                }

                VkDescriptorSetLayoutCreateInfo setLayoutInfo{};
                setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                setLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
                setLayoutInfo.pBindings = bindings.data();
                if (vkCreateDescriptorSetLayout(device, &setLayoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor set layout");
                }

                std::array<VkDescriptorPoolSize, 5> poolSizes =
                {
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32768 },
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32768 },
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 32768 }, 
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32768 },
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8192 },
                };

                VkDescriptorPoolCreateInfo poolInfo{};
                poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                poolInfo.maxSets = 8192;
                poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
                poolInfo.pPoolSizes = poolSizes.data();
                if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create descriptor pool");
                }

                VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
                if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &graphicsPipelineLayout) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create graphics pipeline layout");
                }
            }

            VkPipeline getOrCreateGraphicsPipeline(const DrawCommand &command, VkRenderPass activeRenderPass)
            {
                if (!command.vertexProgram || !command.pixelProgram)
                {
                    return VK_NULL_HANDLE;
                }

                PipelineKey key;
                key.vertexModule = command.vertexProgram->shaderModule;
                key.pixelModule = command.pixelProgram->shaderModule;
                key.inputLayout = command.inputLayout;
                key.renderPass = activeRenderPass;
                key.primitiveType = command.primitiveType;
                key.blendEnabled = (command.blendState ? command.blendState->getDescription().targetStates[0].enable : false);
                key.depthEnabled = (command.depthState ? command.depthState->getDescription().enable : false);
                key.depthWrite = (command.depthState ? (command.depthState->getDescription().writeMask != Render::DepthState::Write::Zero) : false);
                key.depthCompareFunction = (command.depthState ? command.depthState->getDescription().comparisonFunction : Render::ComparisonFunction::Always);
                key.cullMode = (command.renderState ? command.renderState->getDescription().cullMode : Render::RenderState::CullMode::None);
                key.frontCounterClockwise = (command.renderState ? command.renderState->getDescription().frontCounterClockwise : false);

                auto pipelineSearch = graphicsPipelineCache.find(key);
                if (pipelineSearch != std::end(graphicsPipelineCache))
                {
                    return pipelineSearch->second;
                }

                if (key.vertexModule == VK_NULL_HANDLE || key.pixelModule == VK_NULL_HANDLE)
                {
                    return VK_NULL_HANDLE;
                }

                std::vector<VkVertexInputBindingDescription> bindingDescriptions;
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

                if (command.inputLayout)
                {
                    std::array<bool, 8> bindingUsed{};
                    std::array<VkVertexInputRate, 8> bindingRate{};
                    std::array<uint32_t, 8> bindingStride{};

                    std::array<uint32_t, 8> runningOffset{};
                    for (uint32_t index = 0; index < command.inputLayout->elementList.size(); ++index)
                    {
                        const auto &element = command.inputLayout->elementList[index];
                        const uint32_t formatStride = GetFormatStride(element.format);
                        const VkFormat format = GetVkFormat(element.format);
                        if (format == VK_FORMAT_UNDEFINED || formatStride == 0)
                        {
                            continue;
                        }

                        const uint32_t slot = std::min<uint32_t>(element.sourceIndex, 7u);
                        bindingUsed[slot] = true;
                        bindingRate[slot] = (element.source == Render::InputElement::Source::Instance) ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                        if (command.vertexBuffers[slot])
                        {
                            bindingStride[slot] = command.vertexBuffers[slot]->getDescription().stride;
                        }

                        VkVertexInputAttributeDescription attribute{};
                        attribute.location = index;
                        attribute.binding = slot;
                        attribute.format = format;
                        attribute.offset = (element.alignedByteOffset == Render::InputElement::AppendAligned) ? runningOffset[slot] : element.alignedByteOffset;
                        attributeDescriptions.push_back(attribute);
                        runningOffset[slot] = attribute.offset + formatStride;
                    }

                    for (uint32_t slot = 0; slot < bindingUsed.size(); ++slot)
                    {
                        if (!bindingUsed[slot])
                        {
                            continue;
                        }

                        if (bindingStride[slot] == 0)
                        {
                            bindingStride[slot] = runningOffset[slot];
                        }

                        VkVertexInputBindingDescription binding{};
                        binding.binding = slot;
                        binding.stride = bindingStride[slot];
                        binding.inputRate = bindingRate[slot];
                        bindingDescriptions.push_back(binding);
                    }
                }

                VkPipelineShaderStageCreateInfo vertexStage{};
                vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
                vertexStage.module = key.vertexModule;
                vertexStage.pName = "main";

                VkPipelineShaderStageCreateInfo pixelStage{};
                pixelStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                pixelStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                pixelStage.module = key.pixelModule;
                pixelStage.pName = "main";

                VkPipelineShaderStageCreateInfo shaderStages[] = { vertexStage, pixelStage };

                VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
                vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
                vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
                vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
                vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
                vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

                VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
                inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
                inputAssembly.topology = GetVkPrimitiveTopology(key.primitiveType);
                inputAssembly.primitiveRestartEnable = VK_FALSE;

                VkPipelineViewportStateCreateInfo viewportState{};
                viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
                viewportState.viewportCount = 1;
                viewportState.scissorCount = 1;

                VkPipelineRasterizationStateCreateInfo rasterizer{};
                rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
                rasterizer.depthClampEnable = VK_FALSE;
                rasterizer.rasterizerDiscardEnable = VK_FALSE;
                rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
                rasterizer.lineWidth = 1.0f;
                static constexpr VkCullModeFlags vkCullModes[] = { VK_CULL_MODE_NONE, VK_CULL_MODE_FRONT_BIT, VK_CULL_MODE_BACK_BIT };
                rasterizer.cullMode = vkCullModes[static_cast<uint8_t>(key.cullMode)];
                rasterizer.cullMode = VK_CULL_MODE_NONE;
                rasterizer.frontFace = key.frontCounterClockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;

                VkPipelineMultisampleStateCreateInfo multisampling{};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = key.depthEnabled ? VK_TRUE : VK_FALSE;
                depthStencil.depthWriteEnable = key.depthWrite ? VK_TRUE : VK_FALSE;
                static constexpr VkCompareOp vkCompareOps[] =
                {
                    VK_COMPARE_OP_ALWAYS,
                    VK_COMPARE_OP_NEVER,
                    VK_COMPARE_OP_EQUAL,
                    VK_COMPARE_OP_NOT_EQUAL,
                    VK_COMPARE_OP_LESS,
                    VK_COMPARE_OP_LESS_OR_EQUAL,
                    VK_COMPARE_OP_GREATER,
                    VK_COMPARE_OP_GREATER_OR_EQUAL,
                };
                depthStencil.depthCompareOp = key.depthEnabled
                    ? vkCompareOps[static_cast<uint8_t>(key.depthCompareFunction)]
                    : VK_COMPARE_OP_ALWAYS;
                depthStencil.depthBoundsTestEnable = VK_FALSE;
                depthStencil.stencilTestEnable = VK_FALSE;

                VkPipelineColorBlendAttachmentState colorBlendAttachment{};
                colorBlendAttachment.blendEnable = key.blendEnabled ? VK_TRUE : VK_FALSE;
                colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                colorBlendAttachment.colorWriteMask =
                    VK_COLOR_COMPONENT_R_BIT |
                    VK_COLOR_COMPONENT_G_BIT |
                    VK_COLOR_COMPONENT_B_BIT |
                    VK_COLOR_COMPONENT_A_BIT;

                uint32_t colorAttachmentCount = 1;
                if (command.hasOffscreenTarget && command.offscreenTargetCount > 0)
                {
                    colorAttachmentCount = command.offscreenTargetCount;
                }

                std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorAttachmentCount, colorBlendAttachment);

                VkPipelineColorBlendStateCreateInfo colorBlending{};
                colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
                colorBlending.logicOpEnable = VK_FALSE;
                colorBlending.attachmentCount = colorAttachmentCount;
                colorBlending.pAttachments = colorBlendAttachments.data();

                std::array<VkDynamicState, 2> dynamicStates =
                {
                    VK_DYNAMIC_STATE_VIEWPORT,
                    VK_DYNAMIC_STATE_SCISSOR,
                };

                VkPipelineDynamicStateCreateInfo dynamicState{};
                dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
                dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
                dynamicState.pDynamicStates = dynamicStates.data();

                VkGraphicsPipelineCreateInfo pipelineInfo{};
                pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
                pipelineInfo.stageCount = 2;
                pipelineInfo.pStages = shaderStages;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
                pipelineInfo.pViewportState = &viewportState;
                pipelineInfo.pRasterizationState = &rasterizer;
                pipelineInfo.pMultisampleState = &multisampling;
                pipelineInfo.pDepthStencilState = &depthStencil;
                pipelineInfo.pColorBlendState = &colorBlending;
                pipelineInfo.pDynamicState = &dynamicState;
                pipelineInfo.layout = graphicsPipelineLayout;
                pipelineInfo.renderPass = activeRenderPass;
                pipelineInfo.subpass = 0;

                VkPipeline pipeline = VK_NULL_HANDLE;
                const VkResult pipelineResult = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline);
                if (pipelineResult != VK_SUCCESS)
                {
                    if (failedGraphicsPipelineKeys.find(key) == std::end(failedGraphicsPipelineKeys))
                    {
                        failedGraphicsPipelineKeys.insert(key);
                        const auto &vertexInfo = command.vertexProgram->getInformation();
                        const auto &pixelInfo = command.pixelProgram->getInformation();
                        getContext()->log(
                            Gek::Context::Error,
                            "Failed Vulkan graphics pipeline (result={}) vp='{}' pp='{}' vEntry='{}' pEntry='{}' renderPass={} depthEnabled={} depthWrite={} blendEnabled={} attrs={} bindings={}",
                            static_cast<int32_t>(pipelineResult),
                            vertexInfo.name,
                            pixelInfo.name,
                            vertexInfo.entryFunction,
                            pixelInfo.entryFunction,
                            static_cast<uint64_t>(reinterpret_cast<uintptr_t>(activeRenderPass)),
                            static_cast<uint32_t>(key.depthEnabled),
                            static_cast<uint32_t>(key.depthWrite),
                            static_cast<uint32_t>(key.blendEnabled),
                            static_cast<uint32_t>(attributeDescriptions.size()),
                            static_cast<uint32_t>(bindingDescriptions.size()));
                    }
                    return VK_NULL_HANDLE;
                }

                graphicsPipelineCache[key] = pipeline;
                return pipeline;
            }

            VkPipeline getOrCreateComputePipeline(ComputeProgram *program)
            {
                if (!program || program->shaderModule == VK_NULL_HANDLE || graphicsPipelineLayout == VK_NULL_HANDLE)
                {
                    return VK_NULL_HANDLE;
                }

                auto pipelineSearch = computePipelineCache.find(program->shaderModule);
                if (pipelineSearch != std::end(computePipelineCache))
                {
                    return pipelineSearch->second;
                }

                VkPipelineShaderStageCreateInfo stageInfo{};
                stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                stageInfo.module = program->shaderModule;
                stageInfo.pName = "main";

                VkComputePipelineCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                createInfo.stage = stageInfo;
                createInfo.layout = graphicsPipelineLayout;

                VkPipeline pipeline = VK_NULL_HANDLE;
                const VkResult result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
                if (result != VK_SUCCESS)
                {
                    if (failedComputePipelineModules.find(program->shaderModule) == std::end(failedComputePipelineModules))
                    {
                        failedComputePipelineModules.insert(program->shaderModule);
                        getContext()->log(
                            Gek::Context::Error,
                            "Failed Vulkan compute pipeline (result={}) program='{}' entry='{}'",
                            static_cast<int32_t>(result),
                            program->getInformation().name,
                            program->getInformation().entryFunction);
                    }

                    return VK_NULL_HANDLE;
                }

                computePipelineCache[program->shaderModule] = pipeline;
                return pipeline;
            }

            VkFramebuffer getOrCreateOffscreenFramebuffer(VkRenderPass activeRenderPass, const std::vector<VkImageView> &attachments, VkExtent2D extent)
            {
                if (activeRenderPass == VK_NULL_HANDLE || attachments.empty() || extent.width == 0 || extent.height == 0)
                {
                    return VK_NULL_HANDLE;
                }

                FramebufferKey key;
                key.renderPass = activeRenderPass;
                key.extent = extent;
                key.attachmentCount = std::min<uint32_t>(static_cast<uint32_t>(attachments.size()), static_cast<uint32_t>(key.attachments.size()));
                for (uint32_t attachmentIndex = 0; attachmentIndex < key.attachmentCount; ++attachmentIndex)
                {
                    key.attachments[attachmentIndex] = attachments[attachmentIndex];
                }

                auto framebufferSearch = offscreenFramebufferCache.find(key);
                if (framebufferSearch != std::end(offscreenFramebufferCache))
                {
                    return framebufferSearch->second;
                }

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = activeRenderPass;
                framebufferInfo.attachmentCount = key.attachmentCount;
                framebufferInfo.pAttachments = key.attachments.data();
                framebufferInfo.width = extent.width;
                framebufferInfo.height = extent.height;
                framebufferInfo.layers = 1;

                VkFramebuffer framebuffer = VK_NULL_HANDLE;
                if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
                {
                    return VK_NULL_HANDLE;
                }

                offscreenFramebufferCache[key] = framebuffer;
                return framebuffer;
            }

            void createCommandResources(void)
            {
                QueueFamilyIndices indices = findQueueFamilies();

                VkCommandPoolCreateInfo commandPoolInfo{};
                commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();
                if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create command pool!");
                }

                VkCommandPoolCreateInfo uploadCommandPoolInfo{};
                uploadCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                uploadCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                uploadCommandPoolInfo.queueFamilyIndex = indices.graphicsFamily.value();
                if (vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadCommandPool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create upload command pool!");
                }

                VkCommandBufferAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.commandPool = commandPool;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandBufferCount = 1;
                if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to allocate command buffer!");
                }

                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create synchronization semaphores!");
                }

                renderFinishedSemaphores.assign(std::max<size_t>(swapChainImages.size(), 1), VK_NULL_HANDLE);
                for (auto &renderFinished : renderFinishedSemaphores)
                {
                    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinished) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create synchronization semaphores!");
                    }
                }

                VkFenceCreateInfo fenceInfo{};
                fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
                if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create in-flight fence!");
                }
            }

            void destroySwapChainResources(void)
            {
                destroyRenderPassResources();

                for (auto imageView : swapChainImageViews)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                }
                swapChainImageViews.clear();
                swapChainImageLayouts.clear();

                if (swapChain != VK_NULL_HANDLE)
                {
                    vkDestroySwapchainKHR(device, swapChain, nullptr);
                    swapChain = VK_NULL_HANDLE;
                }
            }

            void recreateSwapChain(void)
            {
                vkDeviceWaitIdle(device);
                destroySwapChainResources();
                createSwapChain();
                createImageViews();
                createDepthImage();
                createRenderPassResources();

                VkSemaphoreCreateInfo semaphoreInfo{};
                semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                for (auto &renderFinished : renderFinishedSemaphores)
                {
                    if (renderFinished != VK_NULL_HANDLE)
                    {
                        vkDestroySemaphore(device, renderFinished, nullptr);
                        renderFinished = VK_NULL_HANDLE;
                    }
                }
                renderFinishedSemaphores.assign(std::max<size_t>(swapChainImages.size(), 1), VK_NULL_HANDLE);
                for (auto &renderFinished : renderFinishedSemaphores)
                {
                    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinished) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create synchronization semaphores!");
                    }
                }

                updateBackBufferDescription();
            }

            void updateBackBufferDescription(void)
            {
                Render::Texture::Description description;
                description.name = "BackBuffer";
                description.width = swapChainExtent.width;
                description.height = swapChainExtent.height;
                description.format = Render::Implementation::GetFormat(swapChainImageFormat);
                backBuffer = std::make_unique<Target>(description);

                currentViewport.x = 0.0f;
                currentViewport.y = 0.0f;
                currentViewport.width = static_cast<float>(std::max<uint32_t>(swapChainExtent.width, 1u));
                currentViewport.height = static_cast<float>(std::max<uint32_t>(swapChainExtent.height, 1u));
                currentViewport.minDepth = 0.0f;
                currentViewport.maxDepth = 1.0f;
            }

        public:
            Device(Gek::Context *context, Window::Device *window, Render::Device::Description deviceDescription)
                : ContextRegistration(context)
                , window(window)
            {
                applyBufferVersioningPolicyOverrides(deviceDescription);
                enableValidationLayer = checkValidationLayerSupport();
                createInstance();
                if (enableValidationLayer)
                {
                    setupDebugMessenger();
                }

                createSurface();
                pickPhysicalDevice();
                createLogicalDevice();
                createSwapChain();
                createImageViews();
                createDepthImage();
                createRenderPassResources();
                createCommandResources();
                createDescriptorResources();

                defaultContext = std::make_unique<Context>(this);
                updateBackBufferDescription();

                slang::createGlobalSession(slangGlobalSession.writeRef());
            }

            void setupDebugMessenger(void)
            {
                VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) -> VkBool32 
                {
                    Gek::Context *context = reinterpret_cast<Gek::Context *>(userData);
                    context->log(Gek::Context::Info, callbackData->pMessage);
                    return VK_FALSE;
                };

                createInfo.pUserData = reinterpret_cast<void *>(getContext());
                auto function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
                if (function != nullptr)
                {
                    if (function(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
                    {
                        throw std::runtime_error("Unable to create debug messenger");
                    }
                }
            }

            ~Device(void)
            {
                setFullScreenState(false);
                gVulkanDeviceShuttingDown.store(true, std::memory_order_relaxed);

                if (device != VK_NULL_HANDLE)
                {
                    vkDeviceWaitIdle(device);
                }

                backBuffer = nullptr;
                defaultContext = nullptr;

                if (inFlightFence != VK_NULL_HANDLE)
                {
                    vkDestroyFence(device, inFlightFence, nullptr);
                }

                for (auto &renderFinishedSemaphore : renderFinishedSemaphores)
                {
                    if (renderFinishedSemaphore != VK_NULL_HANDLE)
                    {
                        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
                        renderFinishedSemaphore = VK_NULL_HANDLE;
                    }
                }
                renderFinishedSemaphores.clear();

                if (imageAvailableSemaphore != VK_NULL_HANDLE)
                {
                    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
                }

                if (commandPool != VK_NULL_HANDLE)
                {
                    vkDestroyCommandPool(device, commandPool, nullptr);
                }

                if (uploadCommandPool != VK_NULL_HANDLE)
                {
                    vkDestroyCommandPool(device, uploadCommandPool, nullptr);
                }

                if (graphicsPipelineLayout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
                    graphicsPipelineLayout = VK_NULL_HANDLE;
                }

                if (descriptorPool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                    descriptorPool = VK_NULL_HANDLE;
                }

                if (descriptorSetLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                    descriptorSetLayout = VK_NULL_HANDLE;
                }

                for (auto framebuffer : transientFramebuffers)
                {
                    if (framebuffer != VK_NULL_HANDLE)
                    {
                        vkDestroyFramebuffer(device, framebuffer, nullptr);
                    }
                }
                transientFramebuffers.clear();

                for (auto &offscreenRenderPass : offscreenRenderPassCache)
                {
                    if (offscreenRenderPass.second != VK_NULL_HANDLE)
                    {
                        vkDestroyRenderPass(device, offscreenRenderPass.second, nullptr);
                    }
                }
                offscreenRenderPassCache.clear();

                destroySwapChainResources();
                vkDestroyDevice(device, nullptr);
                device = VK_NULL_HANDLE;

                if (enableValidationLayer)
                {
                    auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
                    if (function != nullptr)
                    {
                        function(instance, debugMessenger, nullptr);
                    }
                }

                vkDestroySurfaceKHR(instance, surface, nullptr);
                vkDestroyInstance(instance, nullptr);
            }

            // Render::Debug::Device
            void * getDevice(void)
            {
                return nullptr;
            }

            // Render::Device
            Render::DisplayModeList getDisplayModeList(Render::Format format) const
            {
                Render::DisplayModeList displayModeList;
#ifdef WIN32
                DEVMODE windowsDisplayMode;
                ZeroMemory(&windowsDisplayMode, sizeof(DEVMODE));
                windowsDisplayMode.dmSize = sizeof(DEVMODE);

                int modeIndex = 0;
                while (EnumDisplaySettings(NULL, modeIndex, &windowsDisplayMode))
                {
                    Render::DisplayMode displayMode(windowsDisplayMode.dmPelsWidth, windowsDisplayMode.dmPelsHeight, Render::Implementation::GetFormat(VK_FORMAT_B8G8R8A8_SRGB));
                    displayMode.refreshRate.numerator = windowsDisplayMode.dmDisplayFrequency;
                    displayMode.refreshRate.denominator = 1;
                    displayModeList.push_back(displayMode);
                    modeIndex++;
                }
#endif
                return displayModeList;
            }

            void setFullScreenState(bool fullScreen)
            {
            }

            void setDisplayMode(const Render::DisplayMode &displayMode)
            {
                getContext()->log(Gek::Context::Info, "Setting display mode: {} x {}", displayMode.width, displayMode.height);
                window->resize(Math::Int2(displayMode.width, displayMode.height));
            }

            void handleResize(void)
            {
                recreateSwapChain();
            }

            Render::Target * const getBackBuffer(void)
            {
                return backBuffer.get();
            }

            Render::Device::Context * const getDefaultContext(void)
            {
                assert(defaultContext);

                return defaultContext.get();
            }

            Render::Device::ContextPtr createDeferredContext(void)
            {
                return std::make_unique<Context>(this, true);
            }

            Render::QueryPtr createQuery(Render::Query::Type type)
            {
                return std::make_unique<Query>();
            }

            Render::RenderStatePtr createRenderState(Render::RenderState::Description const &description)
            {
                return std::make_unique<RenderState>(description);
            }

            Render::DepthStatePtr createDepthState(Render::DepthState::Description const &description)
            {
                return std::make_unique<DepthState>(description);
            }

            Render::BlendStatePtr createBlendState(Render::BlendState::Description const &description)
            {
                return std::make_unique<BlendState>(description);
            }

            Render::SamplerStatePtr createSamplerState(Render::SamplerState::Description const &description)
            {
                auto samplerState = std::make_unique<SamplerState>(device, description);

                static constexpr VkSamplerAddressMode addressModeList[] =
                {
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    VK_SAMPLER_ADDRESS_MODE_REPEAT,
                    VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
                };

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

                const uint8_t filterModeValue = static_cast<uint8_t>(description.filterMode);
                const uint8_t filterGroup = (filterModeValue / 9);
                const uint8_t filterBase = (filterModeValue % 9);
                const bool useAnisotropicFilter = (filterBase == 8);

                switch (filterBase)
                {
                case 0:
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;

                case 1:
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;

                case 2:
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;

                case 3:
                    samplerInfo.minFilter = VK_FILTER_NEAREST;
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;

                case 4:
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;

                case 5:
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.magFilter = VK_FILTER_NEAREST;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;

                case 6:
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
                    break;

                case 7:
                case 8:
                default:
                    samplerInfo.minFilter = VK_FILTER_LINEAR;
                    samplerInfo.magFilter = VK_FILTER_LINEAR;
                    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                    break;
                }

                samplerInfo.addressModeU = addressModeList[static_cast<uint8_t>(description.addressModeU)];
                samplerInfo.addressModeV = addressModeList[static_cast<uint8_t>(description.addressModeV)];
                samplerInfo.addressModeW = addressModeList[static_cast<uint8_t>(description.addressModeW)];
                samplerInfo.anisotropyEnable = (useAnisotropicFilter && samplerAnisotropySupported)
                    ? VK_TRUE
                    : VK_FALSE;
                samplerInfo.maxAnisotropy = samplerInfo.anisotropyEnable
                    ? std::min(maxSamplerAnisotropy, static_cast<float>(std::max(description.maximumAnisotropy, 1u)))
                    : 1.0f;
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                samplerInfo.unnormalizedCoordinates = VK_FALSE;
                samplerInfo.compareEnable = (filterGroup == 1) ? VK_TRUE : VK_FALSE;
                static constexpr VkCompareOp compareOpList[] =
                {
                    VK_COMPARE_OP_ALWAYS,
                    VK_COMPARE_OP_NEVER,
                    VK_COMPARE_OP_EQUAL,
                    VK_COMPARE_OP_NOT_EQUAL,
                    VK_COMPARE_OP_LESS,
                    VK_COMPARE_OP_LESS_OR_EQUAL,
                    VK_COMPARE_OP_GREATER,
                    VK_COMPARE_OP_GREATER_OR_EQUAL,
                };
                samplerInfo.compareOp = compareOpList[static_cast<uint8_t>(description.comparisonFunction)];
                samplerInfo.minLod = description.minimumMipLevel;
                samplerInfo.maxLod = std::isfinite(description.maximumMipLevel)
                    ? std::max(description.maximumMipLevel, description.minimumMipLevel)
                    : VK_LOD_CLAMP_NONE;
                samplerInfo.mipLodBias = description.mipLevelBias;
                if (vkCreateSampler(device, &samplerInfo, nullptr, &samplerState->sampler) != VK_SUCCESS)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan createSamplerState failed: name='{}' minLod={} maxLod={} bias={}",
                        description.name,
                        description.minimumMipLevel,
                        description.maximumMipLevel,
                        description.mipLevelBias);
                    return nullptr;
                }

                return samplerState;
            }

            Render::BufferPtr createBuffer(const Render::Buffer::Description &description, const void *data)
            {
                if (description.count == 0)
                {
                    return nullptr;
                }

                auto buffer = std::make_unique<Buffer>(device, description);
                uint32_t stride = description.stride;
                if (description.format != Render::Format::Unknown && stride == 0)
                {
                    stride = GetFormatStride(description.format);
                }
                if (stride == 0)
                {
                    return nullptr;
                }

                buffer->size = static_cast<VkDeviceSize>(stride) * static_cast<VkDeviceSize>(description.count);

                VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                switch (description.type)
                {
                case Render::Buffer::Type::Vertex:
                    usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                    break;
                case Render::Buffer::Type::Index:
                    usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                    break;
                case Render::Buffer::Type::Constant:
                    usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                    break;
                default:
                    usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    break;
                }

                VkBufferCreateInfo bufferInfo{};
                bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                bufferInfo.size = buffer->size;
                bufferInfo.usage = usage;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                const bool useVersionedConstantBacking = shouldUseVersionedWriteDiscardBuffer(description);
                if (useVersionedConstantBacking)
                {
                    buffer->usesVersionedConstantBacking = true;

                    for (uint32_t slot = 0; slot < Buffer::VersionSlotCount; ++slot)
                    {
                        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->versionBufferList[slot]) != VK_SUCCESS)
                        {
                            return nullptr;
                        }

                        VkMemoryRequirements memoryRequirements{};
                        vkGetBufferMemoryRequirements(device, buffer->versionBufferList[slot], &memoryRequirements);

                        VkMemoryAllocateInfo allocateInfo{};
                        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                        allocateInfo.allocationSize = memoryRequirements.size;
                        allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                        if (allocateInfo.memoryTypeIndex == UINT32_MAX)
                        {
                            getContext()->log(Gek::Context::Error,
                                "No compatible Vulkan memory type found for constant buffer '{}' (typeBits={}, flags={}).",
                                description.name, memoryRequirements.memoryTypeBits,
                                static_cast<uint32_t>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
                            return nullptr;
                        }

                        if (vkAllocateMemory(device, &allocateInfo, nullptr, &buffer->versionMemoryList[slot]) != VK_SUCCESS)
                        {
                            return nullptr;
                        }

                        if (vkBindBufferMemory(device, buffer->versionBufferList[slot], buffer->versionMemoryList[slot], 0) != VK_SUCCESS)
                        {
                            return nullptr;
                        }

                        if ((description.flags & Render::Buffer::Flags::Mappable) != 0)
                        {
                            if (vkMapMemory(device, buffer->versionMemoryList[slot], 0, buffer->size, 0, &buffer->versionMappedDataList[slot]) != VK_SUCCESS)
                            {
                                return nullptr;
                            }
                        }
                    }

                    buffer->activeVersionIndex = 0;
                    buffer->buffer = buffer->versionBufferList[0];
                    buffer->memory = buffer->versionMemoryList[0];
                    buffer->mappedData = buffer->versionMappedDataList[0];
                }
                else
                {
                    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer->buffer) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    VkMemoryRequirements memoryRequirements{};
                    vkGetBufferMemoryRequirements(device, buffer->buffer, &memoryRequirements);

                    VkMemoryAllocateInfo allocateInfo{};
                    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    allocateInfo.allocationSize = memoryRequirements.size;
                    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                    if (allocateInfo.memoryTypeIndex == UINT32_MAX)
                    {
                        getContext()->log(Gek::Context::Error,
                            "No compatible Vulkan memory type found for buffer '{}' (typeBits={}, flags={}).",
                            description.name, memoryRequirements.memoryTypeBits,
                            static_cast<uint32_t>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
                        return nullptr;
                    }
                    if (vkAllocateMemory(device, &allocateInfo, nullptr, &buffer->memory) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    if (vkBindBufferMemory(device, buffer->buffer, buffer->memory, 0) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    if (description.flags & Render::Buffer::Flags::Mappable)
                    {
                        if (vkMapMemory(device, buffer->memory, 0, buffer->size, 0, &buffer->mappedData) != VK_SUCCESS)
                        {
                            return nullptr;
                        }
                    }
                }

                if (data)
                {
                    void *mapData = buffer->mappedData;
                    if (!mapData)
                    {
                        if (vkMapMemory(device, buffer->memory, 0, buffer->size, 0, &mapData) != VK_SUCCESS)
                        {
                            return nullptr;
                        }
                    }

                    std::memcpy(mapData, data, static_cast<size_t>(buffer->size));
                    if (!buffer->mappedData)
                    {
                        vkUnmapMemory(device, buffer->memory);
                    }
                }

                return buffer;
            }

            bool mapBuffer(Render::Buffer *buffer, void *&data, Render::Map mapping)
            {
                auto vulkanBuffer = getObject<Buffer>(buffer);
                if (!vulkanBuffer)
                {
                    return false;
                }

                std::unique_lock<std::mutex> lock(getDrawCommandMutex());

                if (vulkanBuffer->usesVersionedConstantBacking)
                {
                    const uint32_t ringSize = getVersionedRingSize(vulkanBuffer->getDescription());

                    auto selectFreeVersion = [&]() -> std::optional<uint32_t>
                    {
                        for (uint32_t step = 1; step <= ringSize; ++step)
                        {
                            const uint32_t candidateVersion = (vulkanBuffer->activeVersionIndex + step) % ringSize;
                            if (!vulkanBuffer->versionInUseList[candidateVersion])
                            {
                                return candidateVersion;
                            }
                        }

                        return std::nullopt;
                    };

                    if (mapping == Render::Map::WriteDiscard)
                    {
                        auto selectedVersion = selectFreeVersion();
                        if (!selectedVersion)
                        {
                            if (inFlightFencePending)
                            {
                                lock.unlock();
                                const VkResult waitResult = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
                                lock.lock();

                                if (waitResult == VK_SUCCESS)
                                {
                                    inFlightFencePending = false;
                                    releaseVersionedConstantBufferSlots();
                                    selectedVersion = selectFreeVersion();
                                }
                            }
                        }

                        if (selectedVersion)
                        {
                            vulkanBuffer->activeVersionIndex = *selectedVersion;
                        }
                    }

                    vulkanBuffer->buffer = vulkanBuffer->versionBufferList[vulkanBuffer->activeVersionIndex];
                    vulkanBuffer->memory = vulkanBuffer->versionMemoryList[vulkanBuffer->activeVersionIndex];
                    vulkanBuffer->mappedData = vulkanBuffer->versionMappedDataList[vulkanBuffer->activeVersionIndex];
                }

                if (!vulkanBuffer->mappedData)
                {
                    if (vkMapMemory(device, vulkanBuffer->memory, 0, vulkanBuffer->size, 0, &vulkanBuffer->mappedData) != VK_SUCCESS)
                    {
                        return false;
                    }
                }

                data = vulkanBuffer->mappedData;
                return true;
            }

            void unmapBuffer(Render::Buffer *buffer)
            {
                auto vulkanBuffer = getObject<Buffer>(buffer);
                if (vulkanBuffer && !(vulkanBuffer->getDescription().flags & Render::Buffer::Flags::Mappable) && vulkanBuffer->mappedData)
                {
                    std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                    vkUnmapMemory(device, vulkanBuffer->memory);
                    vulkanBuffer->mappedData = nullptr;
                }
            }

            void updateResource(Render::Object *object, const void *data)
            {
                auto vulkanBuffer = getObject<Buffer>(object);
                if (vulkanBuffer && data)
                {
                    std::unique_lock<std::mutex> lock(getDrawCommandMutex());

                    if (vulkanBuffer->usesVersionedConstantBacking)
                    {
                        const uint32_t ringSize = getVersionedRingSize(vulkanBuffer->getDescription());
                        std::optional<uint32_t> selectedVersion;
                        for (uint32_t step = 1; step <= ringSize; ++step)
                        {
                            const uint32_t candidateVersion = (vulkanBuffer->activeVersionIndex + step) % ringSize;
                            if (!vulkanBuffer->versionInUseList[candidateVersion])
                            {
                                selectedVersion = candidateVersion;
                                break;
                            }
                        }

                        if (selectedVersion)
                        {
                            vulkanBuffer->activeVersionIndex = *selectedVersion;
                        }
                        else
                        {
                            if (inFlightFencePending)
                            {
                                lock.unlock();
                                const VkResult waitResult = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
                                lock.lock();

                                if (waitResult == VK_SUCCESS)
                                {
                                    inFlightFencePending = false;
                                    releaseVersionedConstantBufferSlots();

                                    for (uint32_t step = 1; step <= ringSize; ++step)
                                    {
                                        const uint32_t candidateVersion = (vulkanBuffer->activeVersionIndex + step) % ringSize;
                                        if (!vulkanBuffer->versionInUseList[candidateVersion])
                                        {
                                            selectedVersion = candidateVersion;
                                            break;
                                        }
                                    }

                                    if (selectedVersion)
                                    {
                                        vulkanBuffer->activeVersionIndex = *selectedVersion;
                                    }
                                }
                            }
                        }

                        vulkanBuffer->buffer = vulkanBuffer->versionBufferList[vulkanBuffer->activeVersionIndex];
                        vulkanBuffer->memory = vulkanBuffer->versionMemoryList[vulkanBuffer->activeVersionIndex];
                        vulkanBuffer->mappedData = vulkanBuffer->versionMappedDataList[vulkanBuffer->activeVersionIndex];
                    }

                    void *mapData = vulkanBuffer->mappedData;
                    if (!mapData)
                    {
                        if (vkMapMemory(device, vulkanBuffer->memory, 0, vulkanBuffer->size, 0, &mapData) != VK_SUCCESS)
                        {
                            return;
                        }
                    }

                    std::memcpy(mapData, data, static_cast<size_t>(vulkanBuffer->size));
                    if (!vulkanBuffer->mappedData)
                    {
                        vkUnmapMemory(device, vulkanBuffer->memory);
                    }
                }
            }

            void copyResource(Render::Object *destination, Render::Object *source)
            {
                if (!destination || !source)
                {
                    return;
                }

                enqueueCopyResourceCommand(nullptr, destination, source);
            }

            std::string_view const getSemanticMoniker(Render::InputElement::Semantic semantic)
            {
                return SemanticNameList[static_cast<uint8_t>(semantic)];
            }

            Render::ObjectPtr createInputLayout(const std::vector<Render::InputElement> &elementList, Render::Program::Information const &information)
            {
                return std::make_unique<InputLayout>(elementList);
            }

			bool compileProgram(Render::Program::Information &information, std::function<bool(IncludeType, std::string_view, void const **data, uint32_t *size)> &&onInclude = nullptr)
            {
                if (!information.isValid())
                {
                    return false;
                }

                std::function<std::string(std::string_view, uint32_t)> resolveIncludes;
                resolveIncludes = [&](std::string_view source, uint32_t depth) -> std::string
                {
                    if (depth > 32)
                    {
                        return std::string(source);
                    }

                    std::string resolved;
                    resolved.reserve(source.size());

                    auto tryLoadInclude = [&](Render::IncludeType includeType, std::string const &includeName, void const **includeData, uint32_t *includeSize) -> bool
                    {
                        if (!onInclude || includeName.empty())
                        {
                            return false;
                        }

                        auto addUnique = [](std::vector<std::string> &list, std::string const &candidate)
                        {
                            if (candidate.empty())
                            {
                                return;
                            }

                            if (std::find(list.begin(), list.end(), candidate) == list.end())
                            {
                                list.push_back(candidate);
                            }
                        };

                        std::vector<std::string> candidates;
                        addUnique(candidates, includeName);

                        std::string forwardSlashes = includeName;
                        for (char &character : forwardSlashes)
                        {
                            if (character == '\\')
                            {
                                character = '/';
                            }
                        }
                        addUnique(candidates, forwardSlashes);

                        std::string backSlashes = includeName;
                        for (char &character : backSlashes)
                        {
                            if (character == '/')
                            {
                                character = '\\';
                            }
                        }
                        addUnique(candidates, backSlashes);

                        size_t slashPosition = includeName.find_last_of("/\\");
                        size_t extensionPosition = includeName.find_last_of('.');
                        bool hasExtension = (extensionPosition != std::string::npos) && (slashPosition == std::string::npos || extensionPosition > slashPosition);

                        if (!hasExtension)
                        {
                            addUnique(candidates, includeName + ".slang");
                            addUnique(candidates, forwardSlashes + ".slang");
                            addUnique(candidates, backSlashes + ".slang");
                        }

                        for (auto const &candidate : candidates)
                        {
                            if (onInclude(includeType, candidate, includeData, includeSize) && (*includeData) && (*includeSize) > 0)
                            {
                                return true;
                            }
                        }

                        return false;
                    };

                    size_t cursor = 0;
                    while (cursor < source.size())
                    {
                        size_t lineEnd = source.find('\n', cursor);
                        if (lineEnd == std::string_view::npos)
                        {
                            lineEnd = source.size();
                        }

                        std::string_view line = source.substr(cursor, lineEnd - cursor);

                        size_t scan = 0;
                        while (scan < line.size() && (line[scan] == ' ' || line[scan] == '\t'))
                        {
                            ++scan;
                        }

                        size_t includePos = std::string_view::npos;
                        if (scan < line.size() && line[scan] == '#')
                        {
                            ++scan;
                            while (scan < line.size() && (line[scan] == ' ' || line[scan] == '\t'))
                            {
                                ++scan;
                            }

                            if ((scan + 7) <= line.size() && line.substr(scan, 7) == "include")
                            {
                                includePos = scan;
                            }
                        }

                        if (includePos != std::string_view::npos)
                        {
                            size_t openAngle = line.find('<', includePos);
                            size_t closeAngle = (openAngle != std::string_view::npos) ? line.find('>', openAngle + 1) : std::string_view::npos;
                            size_t openQuote = line.find('"', includePos);
                            size_t closeQuote = (openQuote != std::string_view::npos) ? line.find('"', openQuote + 1) : std::string_view::npos;

                            bool isGlobal = (openAngle != std::string_view::npos && closeAngle != std::string_view::npos);
                            bool isLocal = (openQuote != std::string_view::npos && closeQuote != std::string_view::npos);
                            if (isGlobal || isLocal)
                            {
                                size_t begin = isGlobal ? (openAngle + 1) : (openQuote + 1);
                                size_t end = isGlobal ? closeAngle : closeQuote;
                                std::string includeName(line.substr(begin, end - begin));
                                Render::IncludeType includeType = isGlobal ? Render::IncludeType::Global : Render::IncludeType::Local;

                                const void *includeData = nullptr;
                                uint32_t includeSize = 0;
                                bool includeLoaded = tryLoadInclude(includeType, includeName, &includeData, &includeSize);

                                if (includeLoaded && includeData && includeSize > 0)
                                {
                                    std::string_view includeText(reinterpret_cast<const char *>(includeData), includeSize);
                                    resolved += resolveIncludes(includeText, depth + 1);
                                    if (lineEnd < source.size())
                                    {
                                        resolved.push_back('\n');
                                    }
                                    cursor = (lineEnd < source.size()) ? (lineEnd + 1) : lineEnd;
                                    continue;
                                }
                            }
                        }

                        resolved.append(line.data(), line.size());
                        if (lineEnd < source.size())
                        {
                            resolved.push_back('\n');
                        }
                        cursor = (lineEnd < source.size()) ? (lineEnd + 1) : lineEnd;
                    }

                    return resolved;
                };

                std::string resolvedProgram = resolveIncludes(information.shaderData, 0);
                auto normalizeBufferResources = [](std::string &source)
                {
                    const std::string token = "Buffer<";
                    size_t searchPosition = 0;
                    while (true)
                    {
                        size_t position = source.find(token, searchPosition);
                        if (position == std::string::npos)
                        {
                            break;
                        }

                        const bool hasIdentifierPrefix =
                            (position > 0) &&
                            (std::isalnum(static_cast<unsigned char>(source[position - 1])) || source[position - 1] == '_');

                        if (!hasIdentifierPrefix)
                        {
                            source.replace(position, token.size(), "StructuredBuffer<");
                            searchPosition = position + std::string("StructuredBuffer<").size();
                        }
                        else
                        {
                            searchPosition = position + token.size();
                        }
                    }
                };

                normalizeBufferResources(resolvedProgram);
                auto extractRegisterIndex = [](std::string_view line, char registerClass, uint32_t &registerIndex) -> bool
                {
                    std::string token = std::string("register(") + registerClass;
                    size_t registerPos = line.find(token);
                    if (registerPos == std::string_view::npos)
                    {
                        return false;
                    }

                    size_t numberBegin = registerPos + token.size();
                    size_t numberEnd = numberBegin;
                    while (numberEnd < line.size() && std::isdigit(static_cast<unsigned char>(line[numberEnd])))
                    {
                        ++numberEnd;
                    }

                    if (numberBegin == numberEnd)
                    {
                        return false;
                    }

                    registerIndex = static_cast<uint32_t>(std::strtoul(std::string(line.substr(numberBegin, numberEnd - numberBegin)).c_str(), nullptr, 10));
                    return true;
                };

                auto isResourceDeclarationLine = [](std::string_view line) -> bool
                {
                    if (line.find("register(") == std::string_view::npos)
                    {
                        return false;
                    }

                    return (line.find("cbuffer") != std::string_view::npos) ||
                           (line.find("SamplerState") != std::string_view::npos) ||
                           (line.find("Texture") != std::string_view::npos) ||
                           (line.find("StructuredBuffer") != std::string_view::npos) ||
                           (line.find("ByteAddressBuffer") != std::string_view::npos) ||
                           (line.find("RW") != std::string_view::npos) ||
                           (line.find("Buffer<") != std::string_view::npos);
                };

                auto annotateVulkanBindings = [&](std::string &source)
                {
                    std::string annotated;
                    annotated.reserve(source.size() + 1024);

                    size_t cursor = 0;
                    while (cursor < source.size())
                    {
                        size_t lineEnd = source.find('\n', cursor);
                        if (lineEnd == std::string::npos)
                        {
                            lineEnd = source.size();
                        }

                        std::string_view line(source.data() + cursor, lineEnd - cursor);
                        std::string prefix;
                        if (line.find("[[vk::binding(") == std::string_view::npos && isResourceDeclarationLine(line))
                        {
                            uint32_t registerIndex = 0;
                            if (extractRegisterIndex(line, 'b', registerIndex))
                            {
                                const uint32_t uniformBindingBase =
                                    (information.type == Render::Program::Type::Pixel)
                                    ? DescriptorPixelUniformBufferBase
                                    : DescriptorVertexUniformBufferBase;
                                prefix = std::format("[[vk::binding({}, 0)]] ", uniformBindingBase + registerIndex);
                            }
                            else if (extractRegisterIndex(line, 's', registerIndex))
                            {
                                prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorSamplerBase + registerIndex);
                            }
                            else if (extractRegisterIndex(line, 'u', registerIndex))
                            {
                                const bool isStorageImageResource =
                                    (line.find("RWTexture") != std::string_view::npos) ||
                                    (line.find("Texture2D") != std::string_view::npos) ||
                                    (line.find("Texture3D") != std::string_view::npos) ||
                                    (line.find("TextureCube") != std::string_view::npos);
                                const bool isStorageBufferResource =
                                    (line.find("RWStructuredBuffer") != std::string_view::npos) ||
                                    (line.find("RWByteAddressBuffer") != std::string_view::npos) ||
                                    (line.find("RWBuffer") != std::string_view::npos) ||
                                    (line.find("AppendStructuredBuffer") != std::string_view::npos) ||
                                    (line.find("ConsumeStructuredBuffer") != std::string_view::npos);

                                if (isStorageBufferResource && !isStorageImageResource)
                                {
                                    prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorStorageBufferBase + registerIndex);
                                }
                                else
                                {
                                    prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorStorageImageBase + registerIndex);
                                }
                            }
                            else if (extractRegisterIndex(line, 't', registerIndex))
                            {
                                const bool isTextureResource =
                                    (line.find("Texture") != std::string_view::npos) ||
                                    (line.find("Sampler") != std::string_view::npos);
                                const bool isBufferResource =
                                    (line.find("StructuredBuffer") != std::string_view::npos) ||
                                    (line.find("ByteAddressBuffer") != std::string_view::npos) ||
                                    (line.find("RWStructuredBuffer") != std::string_view::npos) ||
                                    (line.find("RWByteAddressBuffer") != std::string_view::npos) ||
                                    (line.find("RWBuffer") != std::string_view::npos) ||
                                    (line.find("Buffer<") != std::string_view::npos) ||
                                    (line.find("Buffer ") != std::string_view::npos) ||
                                    (line.find("Buffer\t") != std::string_view::npos);

                                if (isBufferResource && !isTextureResource)
                                {
                                    prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorStorageBufferBase + registerIndex);
                                }
                                else
                                {
                                    prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorSampledImageBase + registerIndex);
                                }
                            }
                        }

                        annotated += prefix;
                        annotated.append(line.data(), line.size());
                        if (lineEnd < source.size())
                        {
                            annotated.push_back('\n');
                        }

                        cursor = (lineEnd < source.size()) ? (lineEnd + 1) : lineEnd;
                    }

                    source = std::move(annotated);
                };

                annotateVulkanBindings(resolvedProgram);

                slang::TargetDesc targetDesc = {};
                targetDesc.format = SLANG_SPIRV;
                targetDesc.profile = slangGlobalSession->findProfile("spirv_1_5");
                targetDesc.flags = 0;

                slang::SessionDesc sessionDesc = {};
                sessionDesc.targets = &targetDesc;
                sessionDesc.targetCount = 1;
                sessionDesc.compilerOptionEntryCount = 0;

                Slang::ComPtr<slang::ISession> session;
                slangGlobalSession->createSession(sessionDesc, session.writeRef());
                if (!session)
                {
                    getContext()->log(Gek::Context::Error, "Failed to create Slang compilation session");
                    return false;
                }

                const std::string debugFileName(information.shaderPath.getFileName());

                slang::IBlob* outDiagnosticsRaw = nullptr;
                slang::IModule* slangModule = session->loadModuleFromSourceString(information.name.c_str(), debugFileName.c_str(), resolvedProgram.c_str(), &outDiagnosticsRaw);
                Slang::ComPtr<slang::IBlob> outDiagnostics(outDiagnosticsRaw);
                if (!slangModule)
                {
                    const char *diagMsg = outDiagnostics ? reinterpret_cast<const char *>(outDiagnostics->getBufferPointer()) : "Unknown error";
                    getContext()->log(Gek::Context::Error, "Failed to load Slang module from source string: {}", diagMsg);
                    return false;
                }

                Slang::ComPtr<slang::IEntryPoint> entryPoint;
                slangModule->findEntryPointByName(information.entryFunction.c_str(), entryPoint.writeRef());
                if (!entryPoint)
                {
                    getContext()->log(Gek::Context::Error, "Failed to find entry point '{}' in Slang module", information.entryFunction);
                    return false;
                }

                std::vector<slang::IComponentType *> componentTypes;
                componentTypes.push_back(slangModule);
                componentTypes.push_back(entryPoint);

                Slang::ComPtr<slang::IComponentType> composedProgram;
                slang::IBlob* compositeDiagnosticsRaw = nullptr;
                session->createCompositeComponentType(
                    componentTypes.data(),
                    componentTypes.size(),
                    composedProgram.writeRef(),
                    &compositeDiagnosticsRaw);
                Slang::ComPtr<slang::IBlob> compositeDiagnostics(compositeDiagnosticsRaw);
                if (!composedProgram)
                {
                    const char *diagMsg = compositeDiagnostics ? reinterpret_cast<const char *>(compositeDiagnostics->getBufferPointer()) : "Unknown error";
                    getContext()->log(Gek::Context::Error, "Failed to create composite component type for Slang program: {}", diagMsg);
                    return false;
                }

                Slang::ComPtr<slang::IBlob> spirvCode;
                slang::IBlob* spirvDiagnosticsRaw = nullptr;
                composedProgram->getEntryPointCode(
                    0,
                    0,
                    spirvCode.writeRef(),
                    &spirvDiagnosticsRaw);
                Slang::ComPtr<slang::IBlob> spirvDiagnostics(spirvDiagnosticsRaw);
                if (!spirvCode)
                {
                    const char *diagMsg = spirvDiagnostics ? reinterpret_cast<const char *>(spirvDiagnostics->getBufferPointer()) : "Unknown error";
                    getContext()->log(Gek::Context::Error, "Failed to generate SPIRV code for Slang program: {}", diagMsg);
                    return false;
                }

                information.compiledData = spirvCode ? std::vector<uint8_t>((uint8_t*)spirvCode->getBufferPointer(), (uint8_t*)spirvCode->getBufferPointer() + spirvCode->getBufferSize()) : std::vector<uint8_t>();
                return true;
            }

            template <class TYPE>
            Render::ProgramPtr createProgram(Render::Program::Information const &information)
            {
                return std::make_unique<TYPE>(device, information);
            }

            Render::ProgramPtr createProgram(Render::Program::Information const &information)
            {
                switch (information.type)
                {
                case Render::Program::Type::Compute:
                    return createProgram<ComputeProgram>(information);

                case Render::Program::Type::Vertex:
                    return createProgram<VertexProgram>(information);

                case Render::Program::Type::Geometry:
                    return createProgram<GeometryProgram>(information);

                case Render::Program::Type::Pixel:
                    return createProgram<PixelProgram>(information);
                };

                getContext()->log(Gek::Context::Error, "Unknown program pipline encountered");
                return nullptr;
            }

            Render::TexturePtr createNativeSampledTexture2D(
                Render::Texture::Description const &description,
                VkFormat imageFormat,
                uint32_t mipLevelCount,
                std::vector<VkBufferImageCopy> const &copyRegions,
                void const *uploadData,
                VkDeviceSize uploadSize)
            {
                if (imageFormat == VK_FORMAT_UNDEFINED || !uploadData || uploadSize == 0 || copyRegions.empty())
                {
                    return nullptr;
                }

                VkFormatProperties nativeFormatProperties{};
                vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &nativeFormatProperties);
                const VkFormatFeatureFlags requiredFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                if ((nativeFormatProperties.optimalTilingFeatures & requiredFeatures) != requiredFeatures)
                {
                    return nullptr;
                }

                auto texture = std::make_unique<ViewTexture>(description);
                texture->device = device;

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = std::max(description.width, 1u);
                imageInfo.extent.height = std::max(description.height, 1u);
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = std::max(mipLevelCount, 1u);
                imageInfo.arrayLayers = 1;
                imageInfo.format = imageFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateImage(device, &imageInfo, nullptr, &texture->image) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkMemoryRequirements imageMemoryRequirements{};
                vkGetImageMemoryRequirements(device, texture->image, &imageMemoryRequirements);

                VkMemoryAllocateInfo imageAllocInfo{};
                imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                imageAllocInfo.allocationSize = imageMemoryRequirements.size;
                imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (imageAllocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    return nullptr;
                }

                if (vkAllocateMemory(device, &imageAllocInfo, nullptr, &texture->memory) != VK_SUCCESS)
                {
                    return nullptr;
                }

                if (vkBindImageMemory(device, texture->image, texture->memory, 0) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkBuffer stagingBuffer = VK_NULL_HANDLE;
                VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

                VkBufferCreateInfo stagingBufferInfo{};
                stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                stagingBufferInfo.size = uploadSize;
                stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkMemoryRequirements stagingMemoryRequirements{};
                vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemoryRequirements);

                VkMemoryAllocateInfo stagingAllocInfo{};
                stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                stagingAllocInfo.allocationSize = stagingMemoryRequirements.size;
                stagingAllocInfo.memoryTypeIndex = findMemoryType(stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (stagingAllocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    return nullptr;
                }

                if (vkAllocateMemory(device, &stagingAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS)
                {
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    return nullptr;
                }

                if (vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0) != VK_SUCCESS)
                {
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                void *stagingData = nullptr;
                if (vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &stagingData) != VK_SUCCESS)
                {
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                std::memcpy(stagingData, uploadData, static_cast<size_t>(uploadSize));
                vkUnmapMemory(device, stagingMemory);

                std::lock_guard<std::mutex> uploadLock(getUploadCommandPoolMutex());

                VkCommandBufferAllocateInfo commandAllocInfo{};
                commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                commandAllocInfo.commandPool = uploadCommandPool;
                commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                commandAllocInfo.commandBufferCount = 1;

                VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
                if (vkAllocateCommandBuffers(device, &commandAllocInfo, &uploadCommandBuffer) != VK_SUCCESS)
                {
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                if (vkBeginCommandBuffer(uploadCommandBuffer, &beginInfo) != VK_SUCCESS)
                {
                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                VkImageMemoryBarrier toTransfer{};
                toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toTransfer.image = texture->image;
                toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                toTransfer.subresourceRange.baseMipLevel = 0;
                toTransfer.subresourceRange.levelCount = std::max(mipLevelCount, 1u);
                toTransfer.subresourceRange.baseArrayLayer = 0;
                toTransfer.subresourceRange.layerCount = 1;
                toTransfer.srcAccessMask = 0;
                toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(
                    uploadCommandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &toTransfer);

                vkCmdCopyBufferToImage(
                    uploadCommandBuffer,
                    stagingBuffer,
                    texture->image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    static_cast<uint32_t>(copyRegions.size()),
                    copyRegions.data());

                VkImageMemoryBarrier toShaderRead{};
                toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                toShaderRead.image = texture->image;
                toShaderRead.subresourceRange = toTransfer.subresourceRange;
                toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    uploadCommandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &toShaderRead);

                if (vkEndCommandBuffer(uploadCommandBuffer) != VK_SUCCESS)
                {
                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                VkFence uploadFence = VK_NULL_HANDLE;
                VkFenceCreateInfo uploadFenceInfo{};
                uploadFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                if (vkCreateFence(device, &uploadFenceInfo, nullptr, &uploadFence) != VK_SUCCESS)
                {
                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &uploadCommandBuffer;
                VkResult submitResult = VK_ERROR_UNKNOWN;
                {
                    std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                    submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence);
                }

                if (submitResult != VK_SUCCESS)
                {
                    vkDestroyFence(device, uploadFence, nullptr);
                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                constexpr uint64_t kUploadFenceTimeoutNs = 5ull * 1000ull * 1000ull * 1000ull;
                const VkResult waitResult = vkWaitForFences(device, 1, &uploadFence, VK_TRUE, kUploadFenceTimeoutNs);
                vkDestroyFence(device, uploadFence, nullptr);
                if (waitResult != VK_SUCCESS)
                {
                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                    return nullptr;
                }

                vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                vkDestroyBuffer(device, stagingBuffer, nullptr);
                vkFreeMemory(device, stagingMemory, nullptr);

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = texture->image;
                imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewInfo.format = imageFormat;
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewInfo.subresourceRange.baseMipLevel = 0;
                imageViewInfo.subresourceRange.levelCount = std::max(mipLevelCount, 1u);
                imageViewInfo.subresourceRange.baseArrayLayer = 0;
                imageViewInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(device, &imageViewInfo, nullptr, &texture->imageView) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = static_cast<float>(std::max(mipLevelCount, 1u) - 1u);
                if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
                {
                    return nullptr;
                }

                return texture;
            }

            Render::TexturePtr createRenderTargetTexture(Render::Texture::Description const &description)
            {
                auto texture = std::make_unique<UnorderedTargetViewTexture>(description);
                texture->device = device;

                const VkFormat imageFormat = GetVkFormat(description.format);
                if (imageFormat == VK_FORMAT_UNDEFINED)
                {
                    getContext()->log(Gek::Context::Error,
                        "Vulkan createTexture failed: unsupported Render::Format {} for render target '{}'.",
                        static_cast<uint32_t>(description.format), description.name);
                    return nullptr;
                }

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = std::max(description.width, 1u);
                imageInfo.extent.height = std::max(description.height, 1u);
                imageInfo.extent.depth = 1;
                const uint32_t maxMipLevels = (std::bit_width(std::max(imageInfo.extent.width, imageInfo.extent.height)));
                uint32_t mipLevels = (description.mipMapCount == 0)
                    ? std::max(maxMipLevels, 1u)
                    : std::clamp(description.mipMapCount, 1u, std::max(maxMipLevels, 1u));

                VkFormatProperties formatProperties{};
                vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
                const bool supportsBlitSource = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) != 0;
                const bool supportsBlitDestination = (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) != 0;
                texture->supportsMipBlit = (mipLevels > 1) && supportsBlitSource && supportsBlitDestination;
                if (mipLevels > 1 && !texture->supportsMipBlit)
                {
                    mipLevels = 1;
                }

                imageInfo.mipLevels = mipLevels;
                texture->description.mipMapCount = mipLevels;
                imageInfo.arrayLayers = 1;
                imageInfo.format = imageFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateImage(device, &imageInfo, nullptr, &texture->image) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkMemoryRequirements imageMemoryRequirements{};
                vkGetImageMemoryRequirements(device, texture->image, &imageMemoryRequirements);

                VkMemoryAllocateInfo imageAllocInfo{};
                imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                imageAllocInfo.allocationSize = imageMemoryRequirements.size;
                imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (imageAllocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    return nullptr;
                }
                if (vkAllocateMemory(device, &imageAllocInfo, nullptr, &texture->memory) != VK_SUCCESS)
                {
                    return nullptr;
                }

                if (vkBindImageMemory(device, texture->image, texture->memory, 0) != VK_SUCCESS)
                {
                    return nullptr;
                }

                {
                    std::lock_guard<std::mutex> uploadLock(getUploadCommandPoolMutex());

                    VkCommandBufferAllocateInfo commandAllocInfo{};
                    commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    commandAllocInfo.commandPool = uploadCommandPool;
                    commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    commandAllocInfo.commandBufferCount = 1;

                    VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
                    if (vkAllocateCommandBuffers(device, &commandAllocInfo, &uploadCommandBuffer) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    if (vkBeginCommandBuffer(uploadCommandBuffer, &beginInfo) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkImageMemoryBarrier toShaderRead{};
                    toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.image = texture->image;
                    toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    toShaderRead.subresourceRange.baseMipLevel = 0;
                    toShaderRead.subresourceRange.levelCount = mipLevels;
                    toShaderRead.subresourceRange.baseArrayLayer = 0;
                    toShaderRead.subresourceRange.layerCount = 1;
                    toShaderRead.srcAccessMask = 0;
                    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(
                        uploadCommandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &toShaderRead);

                    if (vkEndCommandBuffer(uploadCommandBuffer) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkFence uploadFence = VK_NULL_HANDLE;
                    VkFenceCreateInfo uploadFenceInfo{};
                    uploadFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    if (vkCreateFence(device, &uploadFenceInfo, nullptr, &uploadFence) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkSubmitInfo submitInfo{};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &uploadCommandBuffer;
                    {
                        std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                        const VkResult uploadSubmitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence);
                        if (uploadSubmitResult != VK_SUCCESS)
                        {
                            vkDestroyFence(device, uploadFence, nullptr);
                            vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                            return nullptr;
                        }
                    }

                    constexpr uint64_t kUploadFenceTimeoutNs = 5ull * 1000ull * 1000ull * 1000ull;
                    const VkResult uploadWaitResult = vkWaitForFences(device, 1, &uploadFence, VK_TRUE, kUploadFenceTimeoutNs);
                    vkDestroyFence(device, uploadFence, nullptr);
                    if (uploadWaitResult == VK_TIMEOUT)
                    {
                        getContext()->log(Gek::Context::Error, "Vulkan texture upload wait timed out (staging upload path)");
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    if (uploadWaitResult != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                }

                texture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = texture->image;
                imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewInfo.format = imageFormat;
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewInfo.subresourceRange.baseMipLevel = 0;
                imageViewInfo.subresourceRange.levelCount = mipLevels;
                imageViewInfo.subresourceRange.baseArrayLayer = 0;
                imageViewInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(device, &imageViewInfo, nullptr, &texture->imageView) != VK_SUCCESS)
                {
                    return nullptr;
                }

                {
                    std::lock_guard<std::mutex> lookupLock(persistentImageViewLookupMutex);
                    if (persistentImageViewLookup.size() > 32768)
                    {
                        persistentImageViewLookup.clear();
                    }
                    persistentImageViewLookup[texture->imageView] = std::make_pair(
                        texture->image,
                        VkExtent2D{ std::max(description.width, 1u), std::max(description.height, 1u) });
                }

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = texture->supportsMipBlit
                    ? static_cast<float>(std::max(mipLevels, 1u) - 1u)
                    : 0.0f;
                if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
                {
                    return nullptr;
                }

                return texture;
            }

            Render::TexturePtr createDepthTargetTexture(Render::Texture::Description const &description)
            {
                auto texture = std::make_unique<DepthTexture>(description);
                texture->device = device;

                const VkFormat imageFormat = GetVkFormat(description.format);
                if (imageFormat == VK_FORMAT_UNDEFINED)
                {
                    getContext()->log(Gek::Context::Error,
                        "Vulkan createTexture failed: unsupported depth Render::Format {} for '{}'.",
                        static_cast<uint32_t>(description.format), description.name);
                    return nullptr;
                }

                texture->format = imageFormat;

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = std::max(description.width, 1u);
                imageInfo.extent.height = std::max(description.height, 1u);
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = imageFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateImage(device, &imageInfo, nullptr, &texture->image) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkMemoryRequirements imageMemoryRequirements{};
                vkGetImageMemoryRequirements(device, texture->image, &imageMemoryRequirements);

                VkMemoryAllocateInfo imageAllocInfo{};
                imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                imageAllocInfo.allocationSize = imageMemoryRequirements.size;
                imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (imageAllocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    return nullptr;
                }
                if (vkAllocateMemory(device, &imageAllocInfo, nullptr, &texture->memory) != VK_SUCCESS)
                {
                    return nullptr;
                }

                if (vkBindImageMemory(device, texture->image, texture->memory, 0) != VK_SUCCESS)
                {
                    return nullptr;
                }

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = texture->image;
                imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewInfo.format = imageFormat;
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                imageViewInfo.subresourceRange.baseMipLevel = 0;
                imageViewInfo.subresourceRange.levelCount = 1;
                imageViewInfo.subresourceRange.baseArrayLayer = 0;
                imageViewInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(device, &imageViewInfo, nullptr, &texture->imageView) != VK_SUCCESS)
                {
                    return nullptr;
                }

                {
                    std::lock_guard<std::mutex> lookupLock(persistentImageViewLookupMutex);
                    if (persistentImageViewLookup.size() > 32768)
                    {
                        persistentImageViewLookup.clear();
                    }
                    persistentImageViewLookup[texture->imageView] = std::make_pair(
                        texture->image,
                        VkExtent2D{ std::max(description.width, 1u), std::max(description.height, 1u) });
                }

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = 0.0f;
                if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
                {
                    return nullptr;
                }

                texture->currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                return texture;
            }

            Render::TexturePtr createSampledTexture(Render::Texture::Description const &description, const void *data)
            {
                auto texture = std::make_unique<ViewTexture>(description);
                texture->device = device;

                const VkFormat imageFormat = GetVkFormat(description.format);
                if (imageFormat == VK_FORMAT_UNDEFINED)
                {
                    getContext()->log(Gek::Context::Error,
                        "Vulkan createTexture failed: unsupported Render::Format {} for '{}'.",
                        static_cast<uint32_t>(description.format), description.name);
                    return nullptr;
                }

                VkFormatProperties formatProperties{};
                vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);
                const VkFormatFeatureFlags requiredFeatures = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                if ((formatProperties.optimalTilingFeatures & requiredFeatures) != requiredFeatures)
                {
                    getContext()->log(Gek::Context::Error,
                        "Vulkan createTexture failed: format {} missing sampled/transfer-dst optimal tiling features for '{}' ({}x{}).",
                        static_cast<uint32_t>(imageFormat), description.name, description.width, description.height);
                    return nullptr;
                }

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = std::max(description.width, 1u);
                imageInfo.extent.height = std::max(description.height, 1u);
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = imageFormat;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                const VkResult createImageResult = vkCreateImage(device, &imageInfo, nullptr, &texture->image);
                if (createImageResult != VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Error,
                        "vkCreateImage failed ({}) for '{}' format={} size={}x{} flags={}",
                        static_cast<int32_t>(createImageResult), description.name,
                        static_cast<uint32_t>(imageFormat), imageInfo.extent.width,
                        imageInfo.extent.height, description.flags);
                    return nullptr;
                }

                VkMemoryRequirements imageMemoryRequirements{};
                vkGetImageMemoryRequirements(device, texture->image, &imageMemoryRequirements);

                VkMemoryAllocateInfo imageAllocInfo{};
                imageAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                imageAllocInfo.allocationSize = imageMemoryRequirements.size;
                imageAllocInfo.memoryTypeIndex = findMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (imageAllocInfo.memoryTypeIndex == UINT32_MAX)
                {
                    getContext()->log(Gek::Context::Error,
                        "No compatible Vulkan memory type found for image '{}' (typeBits={}, flags={}).",
                        description.name, imageMemoryRequirements.memoryTypeBits,
                        static_cast<uint32_t>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
                    return nullptr;
                }
                const VkResult imageAllocResult = vkAllocateMemory(device, &imageAllocInfo, nullptr, &texture->memory);
                if (imageAllocResult != VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Error,
                        "vkAllocateMemory for image failed ({}) for '{}' size={} memoryTypeBits={}",
                        static_cast<int32_t>(imageAllocResult), description.name,
                        static_cast<uint64_t>(imageAllocInfo.allocationSize), imageMemoryRequirements.memoryTypeBits);
                    return nullptr;
                }

                const VkResult bindImageResult = vkBindImageMemory(device, texture->image, texture->memory, 0);
                if (bindImageResult != VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Error,
                        "vkBindImageMemory failed ({}) for '{}'", static_cast<int32_t>(bindImageResult), description.name);
                    return nullptr;
                }

                if (data)
                {
                    std::lock_guard<std::mutex> uploadLock(getUploadCommandPoolMutex());

                    const VkDeviceSize uploadSize = static_cast<VkDeviceSize>(std::max(description.width, 1u)) * static_cast<VkDeviceSize>(std::max(description.height, 1u)) * 4u;

                    VkBuffer stagingBuffer = VK_NULL_HANDLE;
                    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

                    VkBufferCreateInfo stagingBufferInfo{};
                    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    stagingBufferInfo.size = uploadSize;
                    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                    if (vkCreateBuffer(device, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    VkMemoryRequirements stagingMemoryRequirements{};
                    vkGetBufferMemoryRequirements(device, stagingBuffer, &stagingMemoryRequirements);

                    VkMemoryAllocateInfo stagingAllocInfo{};
                    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    stagingAllocInfo.allocationSize = stagingMemoryRequirements.size;
                    stagingAllocInfo.memoryTypeIndex = findMemoryType(stagingMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                    if (stagingAllocInfo.memoryTypeIndex == UINT32_MAX)
                    {
                        getContext()->log(Gek::Context::Error,
                            "No compatible Vulkan memory type found for staging buffer '{}' (typeBits={}, flags={}).",
                            description.name, stagingMemoryRequirements.memoryTypeBits,
                            static_cast<uint32_t>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        return nullptr;
                    }
                    if (vkAllocateMemory(device, &stagingAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS)
                    {
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        return nullptr;
                    }

                    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

                    void *stagingData = nullptr;
                    vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &stagingData);
                    std::memcpy(stagingData, data, static_cast<size_t>(uploadSize));
                    vkUnmapMemory(device, stagingMemory);

                    VkCommandBufferAllocateInfo commandAllocInfo{};
                    commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    commandAllocInfo.commandPool = uploadCommandPool;
                    commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    commandAllocInfo.commandBufferCount = 1;

                    VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
                    if (vkAllocateCommandBuffers(device, &commandAllocInfo, &uploadCommandBuffer) != VK_SUCCESS)
                    {
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        vkFreeMemory(device, stagingMemory, nullptr);
                        return nullptr;
                    }

                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    vkBeginCommandBuffer(uploadCommandBuffer, &beginInfo);

                    VkImageMemoryBarrier toTransfer{};
                    toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toTransfer.image = texture->image;
                    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    toTransfer.subresourceRange.baseMipLevel = 0;
                    toTransfer.subresourceRange.levelCount = 1;
                    toTransfer.subresourceRange.baseArrayLayer = 0;
                    toTransfer.subresourceRange.layerCount = 1;
                    toTransfer.srcAccessMask = 0;
                    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    vkCmdPipelineBarrier(
                        uploadCommandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &toTransfer);

                    VkBufferImageCopy region{};
                    region.bufferOffset = 0;
                    region.bufferRowLength = 0;
                    region.bufferImageHeight = 0;
                    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    region.imageSubresource.mipLevel = 0;
                    region.imageSubresource.baseArrayLayer = 0;
                    region.imageSubresource.layerCount = 1;
                    region.imageOffset = { 0, 0, 0 };
                    region.imageExtent = { std::max(description.width, 1u), std::max(description.height, 1u), 1 };
                    vkCmdCopyBufferToImage(uploadCommandBuffer, stagingBuffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                    VkImageMemoryBarrier toShaderRead{};
                    toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.image = texture->image;
                    toShaderRead.subresourceRange = toTransfer.subresourceRange;
                    toShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(
                        uploadCommandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &toShaderRead);

                    if (vkEndCommandBuffer(uploadCommandBuffer) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        vkFreeMemory(device, stagingMemory, nullptr);
                        return nullptr;
                    }

                    VkFence uploadFence = VK_NULL_HANDLE;
                    VkFenceCreateInfo uploadFenceInfo{};
                    uploadFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    if (vkCreateFence(device, &uploadFenceInfo, nullptr, &uploadFence) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        vkFreeMemory(device, stagingMemory, nullptr);
                        return nullptr;
                    }

                    VkSubmitInfo submitInfo{};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &uploadCommandBuffer;
                    {
                        std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                        const VkResult uploadSubmitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence);
                        if (uploadSubmitResult != VK_SUCCESS)
                        {
                            vkDestroyFence(device, uploadFence, nullptr);
                            vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                            vkDestroyBuffer(device, stagingBuffer, nullptr);
                            vkFreeMemory(device, stagingMemory, nullptr);
                            return nullptr;
                        }
                    }

                    constexpr uint64_t kUploadFenceTimeoutNs = 5ull * 1000ull * 1000ull * 1000ull;
                    const VkResult uploadWaitResult = vkWaitForFences(device, 1, &uploadFence, VK_TRUE, kUploadFenceTimeoutNs);
                    vkDestroyFence(device, uploadFence, nullptr);
                    if (uploadWaitResult == VK_TIMEOUT)
                    {
                        getContext()->log(Gek::Context::Error, "Vulkan texture upload wait timed out (copy-to-image path)");
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        vkFreeMemory(device, stagingMemory, nullptr);
                        return nullptr;
                    }

                    if (uploadWaitResult != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        vkDestroyBuffer(device, stagingBuffer, nullptr);
                        vkFreeMemory(device, stagingMemory, nullptr);
                        return nullptr;
                    }

                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                    vkDestroyBuffer(device, stagingBuffer, nullptr);
                    vkFreeMemory(device, stagingMemory, nullptr);
                }
                else
                {
                    std::lock_guard<std::mutex> uploadLock(getUploadCommandPoolMutex());

                    VkCommandBufferAllocateInfo commandAllocInfo{};
                    commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                    commandAllocInfo.commandPool = uploadCommandPool;
                    commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                    commandAllocInfo.commandBufferCount = 1;

                    VkCommandBuffer uploadCommandBuffer = VK_NULL_HANDLE;
                    if (vkAllocateCommandBuffers(device, &commandAllocInfo, &uploadCommandBuffer) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    if (vkBeginCommandBuffer(uploadCommandBuffer, &beginInfo) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkImageMemoryBarrier toShaderRead{};
                    toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    toShaderRead.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    toShaderRead.image = texture->image;
                    toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    toShaderRead.subresourceRange.baseMipLevel = 0;
                    toShaderRead.subresourceRange.levelCount = 1;
                    toShaderRead.subresourceRange.baseArrayLayer = 0;
                    toShaderRead.subresourceRange.layerCount = 1;
                    toShaderRead.srcAccessMask = 0;
                    toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(
                        uploadCommandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &toShaderRead);

                    if (vkEndCommandBuffer(uploadCommandBuffer) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkFence uploadFence = VK_NULL_HANDLE;
                    VkFenceCreateInfo uploadFenceInfo{};
                    uploadFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
                    if (vkCreateFence(device, &uploadFenceInfo, nullptr, &uploadFence) != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    VkSubmitInfo submitInfo{};
                    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    submitInfo.commandBufferCount = 1;
                    submitInfo.pCommandBuffers = &uploadCommandBuffer;
                    {
                        std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                        const VkResult uploadSubmitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, uploadFence);
                        if (uploadSubmitResult != VK_SUCCESS)
                        {
                            vkDestroyFence(device, uploadFence, nullptr);
                            vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                            return nullptr;
                        }
                    }

                    constexpr uint64_t kUploadFenceTimeoutNs = 5ull * 1000ull * 1000ull * 1000ull;
                    const VkResult uploadWaitResult = vkWaitForFences(device, 1, &uploadFence, VK_TRUE, kUploadFenceTimeoutNs);
                    vkDestroyFence(device, uploadFence, nullptr);
                    if (uploadWaitResult == VK_TIMEOUT)
                    {
                        getContext()->log(Gek::Context::Error, "Vulkan texture upload wait timed out (render-target init path)");
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    if (uploadWaitResult != VK_SUCCESS)
                    {
                        vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                        return nullptr;
                    }

                    vkFreeCommandBuffers(device, uploadCommandPool, 1, &uploadCommandBuffer);
                }

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = texture->image;
                imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewInfo.format = imageFormat;
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewInfo.subresourceRange.baseMipLevel = 0;
                imageViewInfo.subresourceRange.levelCount = 1;
                imageViewInfo.subresourceRange.baseArrayLayer = 0;
                imageViewInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(device, &imageViewInfo, nullptr, &texture->imageView) != VK_SUCCESS)
                {
                    return nullptr;
                }

                {
                    std::lock_guard<std::mutex> lookupLock(persistentImageViewLookupMutex);
                    if (persistentImageViewLookup.size() > 32768)
                    {
                        persistentImageViewLookup.clear();
                    }
                    persistentImageViewLookup[texture->imageView] = std::make_pair(
                        texture->image,
                        VkExtent2D{ std::max(description.width, 1u), std::max(description.height, 1u) });
                }

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = 0.0f;
                if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
                {
                    return nullptr;
                }

                return texture;
            }

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data)
            {
                if (description.flags & Render::Texture::Flags::RenderTarget)
                {
                    return createRenderTargetTexture(description);
                }

                if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    return createDepthTargetTexture(description);
                }

                return createSampledTexture(description, data);
            }

            Render::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                std::lock_guard<std::mutex> decodeLock(getTextureDecodeMutex());

                auto createAllocationFailureFallback = [&](std::string_view sourceName) -> Render::TexturePtr
                {
                    constexpr uint32_t fallbackWidth = 2;
                    constexpr uint32_t fallbackHeight = 2;
                    constexpr std::array<uint8_t, fallbackWidth * fallbackHeight * 4> fallbackPixels =
                    {
                        255, 0, 255, 255,
                        0, 0, 0, 255,
                        0, 0, 0, 255,
                        255, 0, 255, 255,
                    };

                    Render::Texture::Description fallbackDescription;
                    fallbackDescription.name = std::string(sourceName);
                    fallbackDescription.format = (flags & Render::TextureLoadFlags::sRGB)
                        ? Render::Format::R8G8B8A8_UNORM_SRGB
                        : Render::Format::R8G8B8A8_UNORM;
                    fallbackDescription.width = fallbackWidth;
                    fallbackDescription.height = fallbackHeight;
                    fallbackDescription.depth = 1;
                    fallbackDescription.mipMapCount = 1;
                    fallbackDescription.flags = Render::Texture::Flags::Resource;

                    if (auto fallbackTexture = createTexture(fallbackDescription, fallbackPixels.data()))
                    {
                        getContext()->log(
                            Gek::Context::Warning,
                            "Vulkan texture allocation fallback: using 2x2 placeholder for '{}'",
                            sourceName);
                        return fallbackTexture;
                    }

                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan texture allocation fallback failed for '{}'",
                        sourceName);
                    return nullptr;
                };

                std::vector<uint8_t> sourceData = FileSystem::Load(filePath);
                if (sourceData.empty())
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: unable to read file '{}'", filePath.getString());
                    return nullptr;
                }

                std::string extension = String::GetLower(filePath.getExtension());
                ::DirectX::ScratchImage decodedImage;
                HRESULT decodeResult = E_FAIL;

#ifdef WIN32
                struct ScopedComInitialize
                {
                    bool shouldUninitialize = false;

                    ScopedComInitialize(void)
                    {
                        const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                        if (initializeResult == S_OK || initializeResult == S_FALSE)
                        {
                            shouldUninitialize = true;
                        }
                    }

                    ~ScopedComInitialize(void)
                    {
                        if (shouldUninitialize)
                        {
                            CoUninitialize();
                        }
                    }
                } scopedComInitialize;
#endif

                // KTX2 loading using libktx
                if (extension == ".ktx2") {
                    ktxTexture2* kTexture = nullptr;
                    KTX_error_code ktxResult = ktxTexture2_CreateFromMemory(sourceData.data(), sourceData.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
                    if (ktxResult != KTX_SUCCESS || !kTexture) {
                        getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: KTX2 decode error for '{}'", filePath.getString());
                        return nullptr;
                    }

                    VkFormat vkFormat = ktxTexture_GetVkFormat(ktxTexture(kTexture));
                    if (vkFormat == VK_FORMAT_UNDEFINED) {
                        getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: unsupported KTX2 format for '{}'", filePath.getString());
                        ktxTexture_Destroy(ktxTexture(kTexture));
                        return nullptr;
                    }

                    uint32_t mipLevels = kTexture->numLevels;
                    uint32_t width = kTexture->baseWidth;
                    uint32_t height = kTexture->baseHeight;

                    // Prepare upload data for all mip levels
                    std::vector<VkBufferImageCopy> copyRegions;
                    std::vector<uint8_t> uploadData;
                    VkDeviceSize runningOffset = 0;
                    for (uint32_t mip = 0; mip < mipLevels; ++mip) {
                        ktx_size_t offset, size;
                        ktxTexture_GetImageOffset(ktxTexture(kTexture), mip, 0, 0, &offset);
                        size = ktxTexture_GetImageSize(ktxTexture(kTexture), mip);
                        const uint8_t* mipData = kTexture->pData + offset;
                        size_t oldSize = uploadData.size();
                        uploadData.resize(oldSize + size);
                        std::memcpy(uploadData.data() + oldSize, mipData, size);

                        VkBufferImageCopy region{};
                        region.bufferOffset = runningOffset;
                        region.bufferRowLength = 0;
                        region.bufferImageHeight = 0;
                        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        region.imageSubresource.mipLevel = mip;
                        region.imageSubresource.baseArrayLayer = 0;
                        region.imageSubresource.layerCount = 1;
                        region.imageOffset = { 0, 0, 0 };
                        region.imageExtent = { std::max(width >> mip, 1u), std::max(height >> mip, 1u), 1 };
                        copyRegions.push_back(region);
                        runningOffset += size;
                    }

                    Render::Texture::Description nativeDescription;
                    nativeDescription.name = filePath.getString();
                    nativeDescription.format = GetFormat(vkFormat);
                    nativeDescription.width = width;
                    nativeDescription.height = height;
                    nativeDescription.depth = 1;
                    nativeDescription.mipMapCount = mipLevels;
                    nativeDescription.flags = Render::Texture::Flags::Resource;

                    auto texture = createNativeSampledTexture2D(
                        nativeDescription,
                        vkFormat,
                        mipLevels,
                        copyRegions,
                        uploadData.data(),
                        static_cast<VkDeviceSize>(uploadData.size()));
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    if (texture) {
                        return texture;
                    }
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: could not create texture from KTX2 for '{}'", filePath.getString());
                    return nullptr;
                }
                // ...existing code for fallback/other formats...
            }

            Render::TexturePtr loadTexture(void const *buffer, size_t size, uint32_t flags)
            {
                std::lock_guard<std::mutex> decodeLock(getTextureDecodeMutex());

                auto createAllocationFailureFallback = [&](std::string_view sourceName) -> Render::TexturePtr
                {
                    constexpr uint32_t fallbackWidth = 2;
                    constexpr uint32_t fallbackHeight = 2;
                    constexpr std::array<uint8_t, fallbackWidth * fallbackHeight * 4> fallbackPixels =
                    {
                        255, 0, 255, 255,
                        0, 0, 0, 255,
                        0, 0, 0, 255,
                        255, 0, 255, 255,
                    };

                    Render::Texture::Description fallbackDescription;
                    fallbackDescription.name = std::string(sourceName);
                    fallbackDescription.format = (flags & Render::TextureLoadFlags::sRGB)
                        ? Render::Format::R8G8B8A8_UNORM_SRGB
                        : Render::Format::R8G8B8A8_UNORM;
                    fallbackDescription.width = fallbackWidth;
                    fallbackDescription.height = fallbackHeight;
                    fallbackDescription.depth = 1;
                    fallbackDescription.mipMapCount = 1;
                    fallbackDescription.flags = Render::Texture::Flags::Resource;

                    if (auto fallbackTexture = createTexture(fallbackDescription, fallbackPixels.data()))
                    {
                        getContext()->log(
                            Gek::Context::Warning,
                            "Vulkan texture allocation fallback: using 2x2 placeholder for '{}'",
                            sourceName);
                        return fallbackTexture;
                    }

                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan texture allocation fallback failed for '{}'",
                        sourceName);
                    return nullptr;
                };

                if (!buffer || size == 0)
                {
                    return nullptr;
                }

                ::DirectX::ScratchImage decodedImage;

#ifdef WIN32
                struct ScopedComInitialize
                {
                    bool shouldUninitialize = false;

                    ScopedComInitialize(void)
                    {
                        const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                        if (initializeResult == S_OK || initializeResult == S_FALSE)
                        {
                            shouldUninitialize = true;
                        }
                    }

                    ~ScopedComInitialize(void)
                    {
                        if (shouldUninitialize)
                        {
                            CoUninitialize();
                        }
                    }
                } scopedComInitialize;
#endif

                HRESULT decodeResult = ::DirectX::LoadFromDDSMemory(reinterpret_cast<const std::byte *>(buffer), size, ::DirectX::DDS_FLAGS_NONE, nullptr, decodedImage);
                if (FAILED(decodeResult))
                {
                    decodeResult = ::DirectX::LoadFromTGAMemory(reinterpret_cast<const uint8_t *>(buffer), size, nullptr, decodedImage);
                }

                if (FAILED(decodeResult))
                {
                    auto wicFlags = (flags & Render::TextureLoadFlags::sRGB) ? ::DirectX::WIC_FLAGS_NONE : ::DirectX::WIC_FLAGS_IGNORE_SRGB;
                    wicFlags |= ::DirectX::WIC_FLAGS_FORCE_RGB;
                    decodeResult = ::DirectX::LoadFromWICMemory(reinterpret_cast<const std::byte *>(buffer), size, wicFlags, nullptr, decodedImage);
                }

                if (FAILED(decodeResult))
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed: decode error");
                    return nullptr;
                }

                const auto metadata = decodedImage.GetMetadata();
                if (metadata.width == 0 || metadata.height == 0 || metadata.depth == 0)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed: invalid metadata");
                    return nullptr;
                }

                const DXGI_FORMAT targetFormat = (flags & Render::TextureLoadFlags::sRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                ::DirectX::ScratchImage convertedImage;
                const ::DirectX::Image *image = nullptr;
                if (metadata.format == targetFormat)
                {
                    image = decodedImage.GetImage(0, 0, 0);
                }
                else
                {
                    if (FAILED(::DirectX::Convert(decodedImage.GetImages(), decodedImage.GetImageCount(), metadata, targetFormat, ::DirectX::TEX_FILTER_DEFAULT, ::DirectX::TEX_THRESHOLD_DEFAULT, convertedImage)))
                    {
                        getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed: RGBA conversion error");
                        return nullptr;
                    }

                    image = convertedImage.GetImage(0, 0, 0);
                }
                if (!image || !image->pixels)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed: converted image missing pixels");
                    return nullptr;
                }

                const uint32_t width = static_cast<uint32_t>(image->width);
                const uint32_t height = static_cast<uint32_t>(image->height);
                if (width == 0 || height == 0)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed: converted image has zero extent");
                    return nullptr;
                }

                const uint32_t uploadWidth = width;
                const uint32_t uploadHeight = height;

                const size_t requiredRowPitch = static_cast<size_t>(uploadWidth) * 4u;
                if (image->rowPitch < requiredRowPitch)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan loadTexture(memory) failed: converted row pitch too small (rowPitch={} required={})",
                        static_cast<uint64_t>(image->rowPitch),
                        static_cast<uint64_t>(requiredRowPitch));
                    return nullptr;
                }

                std::vector<uint8_t> uploadData(static_cast<size_t>(uploadWidth) * static_cast<size_t>(uploadHeight) * 4u);
                for (uint32_t row = 0; row < uploadHeight; ++row)
                {
                    const uint8_t *sourceRow = image->pixels + (static_cast<size_t>(row) * image->rowPitch);
                    uint8_t *destRow = uploadData.data() + (static_cast<size_t>(row) * requiredRowPitch);
                    std::memcpy(destRow, sourceRow, requiredRowPitch);
                }

                Render::Texture::Description description;
                description.name = "memory_texture";
                description.format = (flags & Render::TextureLoadFlags::sRGB)
                    ? Render::Format::R8G8B8A8_UNORM_SRGB
                    : Render::Format::R8G8B8A8_UNORM;
                description.width = uploadWidth;
                description.height = uploadHeight;
                description.depth = 1;
                description.mipMapCount = 1;
                description.flags = Render::Texture::Flags::Resource;

                if (auto texture = createTexture(description, uploadData.data()))
                {
                    return texture;
                }

                getContext()->log(Gek::Context::Error, "Vulkan loadTexture(memory) failed after retries");
                return createAllocationFailureFallback("memory_texture");
            }

            Texture::Description loadTextureDescription(FileSystem::Path const &filePath)
            {
                std::lock_guard<std::mutex> decodeLock(getTextureDecodeMutex());

                Texture::Description description;
                std::vector<uint8_t> sourceData = FileSystem::Load(filePath, 1024 * 4);
                if (sourceData.empty())
                {
                    return description;
                }

                ::DirectX::TexMetadata metadata{};
                std::string extension = String::GetLower(filePath.getExtension());

#ifdef WIN32
                struct ScopedComInitialize
                {
                    bool shouldUninitialize = false;

                    ScopedComInitialize(void)
                    {
                        const HRESULT initializeResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
                        if (initializeResult == S_OK || initializeResult == S_FALSE)
                        {
                            shouldUninitialize = true;
                        }
                    }

                    ~ScopedComInitialize(void)
                    {
                        if (shouldUninitialize)
                        {
                            CoUninitialize();
                        }
                    }
                } scopedComInitialize;
#endif

                HRESULT metadataResult = E_FAIL;
                if (extension == ".dds")
                {
                    metadataResult = ::DirectX::GetMetadataFromDDSMemory(sourceData.data(), sourceData.size(), ::DirectX::DDS_FLAGS_NONE, metadata);
                }
                else if (extension == ".tga")
                {
                    metadataResult = ::DirectX::GetMetadataFromTGAMemory(sourceData.data(), sourceData.size(), metadata);
                }
                else
                {
                    metadataResult = ::DirectX::GetMetadataFromWICMemory(sourceData.data(), sourceData.size(), ::DirectX::WIC_FLAGS_NONE, metadata);
                }

                if (FAILED(metadataResult))
                {
                    return description;
                }

                description.name = filePath.getString();
                description.format = Render::Format::R8G8B8A8_UNORM;
                description.width = static_cast<uint32_t>(metadata.width);
                description.height = static_cast<uint32_t>(metadata.height);
                description.depth = static_cast<uint32_t>(std::max<size_t>(metadata.depth, 1));
                description.mipMapCount = static_cast<uint32_t>(std::max<size_t>(metadata.mipLevels, 1));
                description.flags = Render::Texture::Flags::Resource;
                return description;
            }

            void executeCommandList(Render::Object *commandList)
            {
                auto *vkCommandList = getObject<CommandList>(commandList);
                if (!vkCommandList || vkCommandList->identifier == 0)
                {
                    return;
                }

                std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                auto deferredListIterator = deferredCommandLists.find(vkCommandList->identifier);
                if (deferredListIterator == deferredCommandLists.end())
                {
                    return;
                }

                pendingDrawCommands.insert(pendingDrawCommands.end(), deferredListIterator->second.begin(), deferredListIterator->second.end());
                deferredCommandLists.erase(deferredListIterator);
            }

            void present(bool waitForVerticalSync)
            {
                const auto frameCpuStartTime = std::chrono::high_resolution_clock::now();
                double waitFenceCpuMs = 0.0;
                double acquireCpuMs = 0.0;
                double recordCpuMs = 0.0;
                double cleanupCpuMs = 0.0;
                double cleanupFramebufferCpuMs = 0.0;
                double cleanupDescriptorCpuMs = 0.0;
                double drawCommandLockCpuMs = 0.0;

                if (device == VK_NULL_HANDLE || swapChain == VK_NULL_HANDLE)
                {
                    return;
                }

                if (deviceLost)
                {
                    std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                    pendingDrawCommands.clear();
                    return;
                }

                auto clearQueuedDrawCommands = [&]()
                {
                    std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                    pendingDrawCommands.clear();
                };

                auto handleDeviceLost = [&](VkResult errorCode, std::string_view operation)
                {
                    if (!loggedDeviceLost)
                    {
                        loggedDeviceLost = true;
                        getContext()->log(
                            Gek::Context::Error,
                            "Vulkan device lost during {}: result={}. Rendering is suspended until restart.",
                            operation,
                            static_cast<int32_t>(errorCode));
                    }

                    deviceLost = true;
                    clearQueuedDrawCommands();
                };

                if (inFlightFencePending)
                {
                    const auto waitFenceStartTime = std::chrono::high_resolution_clock::now();
                    const VkResult frameWaitResult = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
                    const auto waitFenceEndTime = std::chrono::high_resolution_clock::now();
                    waitFenceCpuMs = std::chrono::duration<double, std::milli>(waitFenceEndTime - waitFenceStartTime).count();
                    if (frameWaitResult != VK_SUCCESS)
                    {
                        handleDeviceLost(frameWaitResult, "vkWaitForFences(inFlightFence)");
                        return;
                    }

                    inFlightFencePending = false;

                    {
                        std::lock_guard<std::mutex> drawLock(getDrawCommandMutex());
                        releaseVersionedConstantBufferSlots();
                    }
                }

                const auto cleanupStartTime = std::chrono::high_resolution_clock::now();

                const auto cleanupFramebufferStartTime = std::chrono::high_resolution_clock::now();
                for (auto framebuffer : transientFramebuffers)
                {
                    if (framebuffer != VK_NULL_HANDLE)
                    {
                        vkDestroyFramebuffer(device, framebuffer, nullptr);
                    }
                }

                transientFramebuffers.clear();
                const auto cleanupFramebufferEndTime = std::chrono::high_resolution_clock::now();
                cleanupFramebufferCpuMs = std::chrono::duration<double, std::milli>(cleanupFramebufferEndTime - cleanupFramebufferStartTime).count();

                const auto cleanupDescriptorStartTime = std::chrono::high_resolution_clock::now();
                if (descriptorPool != VK_NULL_HANDLE)
                {
                    vkResetDescriptorPool(device, descriptorPool, 0);
                }

                const auto cleanupDescriptorEndTime = std::chrono::high_resolution_clock::now();
                cleanupDescriptorCpuMs = std::chrono::duration<double, std::milli>(cleanupDescriptorEndTime - cleanupDescriptorStartTime).count();

                const auto cleanupEndTime = std::chrono::high_resolution_clock::now();
                cleanupCpuMs = std::chrono::duration<double, std::milli>(cleanupEndTime - cleanupStartTime).count();

                uint32_t imageIndex = 0;
                const auto acquireStartTime = std::chrono::high_resolution_clock::now();
                VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
                const auto acquireEndTime = std::chrono::high_resolution_clock::now();
                acquireCpuMs = std::chrono::duration<double, std::milli>(acquireEndTime - acquireStartTime).count();
                if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
                {
                    clearQueuedDrawCommands();
                    recreateSwapChain();
                    return;
                }
                else if (acquireResult == VK_ERROR_DEVICE_LOST)
                {
                    handleDeviceLost(acquireResult, "vkAcquireNextImageKHR");
                    return;
                }
                else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan acquire swap-chain image failed: result={}",
                        static_cast<int32_t>(acquireResult));
                    clearQueuedDrawCommands();
                    recreateSwapChain();
                    return;
                }

                if (renderFinishedSemaphores.empty() || imageIndex >= renderFinishedSemaphores.size())
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan render-finished semaphore state invalid: imageIndex={} semaphoreCount={}",
                        imageIndex,
                        static_cast<uint32_t>(renderFinishedSemaphores.size()));
                    clearQueuedDrawCommands();
                    recreateSwapChain();
                    return;
                }

                auto transitionSwapChainImage = [&](uint32_t trackedImageIndex, VkImageLayout newLayout, VkAccessFlags dstAccessMask, VkPipelineStageFlags dstStageMask)
                {
                    if (trackedImageIndex >= swapChainImages.size() || trackedImageIndex >= swapChainImageLayouts.size())
                    {
                        return;
                    }

                    const VkImageLayout oldLayout = swapChainImageLayouts[trackedImageIndex];
                    if (oldLayout == newLayout)
                    {
                        return;
                    }

                    VkImageMemoryBarrier barrier{};
                    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    barrier.oldLayout = oldLayout;
                    barrier.newLayout = newLayout;
                    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barrier.image = swapChainImages[trackedImageIndex];
                    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barrier.subresourceRange.baseMipLevel = 0;
                    barrier.subresourceRange.levelCount = 1;
                    barrier.subresourceRange.baseArrayLayer = 0;
                    barrier.subresourceRange.layerCount = 1;

                    VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    switch (oldLayout)
                    {
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                        srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                        break;

                    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                        barrier.srcAccessMask = 0;
                        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;

                    default:
                        barrier.srcAccessMask = 0;
                        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        break;
                    }

                    barrier.dstAccessMask = dstAccessMask;

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        srcStageMask,
                        dstStageMask,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);

                    swapChainImageLayouts[trackedImageIndex] = newLayout;
                };

                const auto recordStartTime = std::chrono::high_resolution_clock::now();
                vkResetCommandBuffer(commandBuffer, 0);

                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to begin recording command buffer!");
                }

                transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                VkImageSubresourceRange clearRange{};
                clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                clearRange.baseMipLevel = 0;
                clearRange.levelCount = 1;
                clearRange.baseArrayLayer = 0;
                clearRange.layerCount = 1;
                vkCmdClearColorImage(commandBuffer, swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &pendingClearColor, 1, &clearRange);

                // Clear depth image once per frame before any render passes use it
                if (depthImage != VK_NULL_HANDLE)
                {
                    VkImageSubresourceRange depthRange{};
                    depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                    depthRange.baseMipLevel = 0;
                    depthRange.levelCount = 1;
                    depthRange.baseArrayLayer = 0;
                    depthRange.layerCount = 1;

                    VkImageMemoryBarrier depthToClear{};
                    depthToClear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    depthToClear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                    depthToClear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    depthToClear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    depthToClear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    depthToClear.image = depthImage;
                    depthToClear.subresourceRange = depthRange;
                    depthToClear.srcAccessMask = 0;
                    depthToClear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &depthToClear);

                    VkClearDepthStencilValue clearDepthValue{ 1.0f, 0 };
                    vkCmdClearDepthStencilImage(commandBuffer, depthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearDepthValue, 1, &depthRange);

                    VkImageMemoryBarrier depthToAttachment{};
                    depthToAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    depthToAttachment.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    depthToAttachment.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    depthToAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    depthToAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    depthToAttachment.image = depthImage;
                    depthToAttachment.subresourceRange = depthRange;
                    depthToAttachment.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    depthToAttachment.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &depthToAttachment);
                }

                const auto drawCommandLockStartTime = std::chrono::high_resolution_clock::now();
                std::lock_guard<std::mutex> drawCommandLock(getDrawCommandMutex());
                const auto drawCommandLockEndTime = std::chrono::high_resolution_clock::now();
                drawCommandLockCpuMs = std::chrono::duration<double, std::milli>(drawCommandLockEndTime - drawCommandLockStartTime).count();
                if (offscreenImageLayouts.size() > 32768)
                {
                    offscreenImageLayouts.clear();
                }

                const bool hasDrawCommands = !pendingDrawCommands.empty();
                std::vector<VkDescriptorSet> descriptorSets;
                descriptorSets.reserve(pendingDrawCommands.size());
                struct GraphicsDescriptorSignature
                {
                    std::array<VkImageView, PixelResourceSlotCount> pixelResourceImageViews{};
                    std::array<VkSampler, PixelResourceSlotCount> pixelSamplerStates{};
                    std::array<Buffer *, PixelResourceSlotCount> pixelResourceBuffers{};
                    std::array<Buffer *, PixelResourceSlotCount> pixelConstantBuffers{};
                    std::array<Buffer *, PixelResourceSlotCount> vertexConstantBuffers{};

                    bool operator == (const GraphicsDescriptorSignature &other) const
                    {
                        return
                            (pixelResourceImageViews == other.pixelResourceImageViews) &&
                            (pixelSamplerStates == other.pixelSamplerStates) &&
                            (pixelResourceBuffers == other.pixelResourceBuffers) &&
                            (pixelConstantBuffers == other.pixelConstantBuffers) &&
                            (vertexConstantBuffers == other.vertexConstantBuffers);
                    }
                };
                GraphicsDescriptorSignature lastGraphicsDescriptorSignature{};
                bool hasLastGraphicsDescriptorSignature = false;
                VkDescriptorSet lastGraphicsDescriptorSet = VK_NULL_HANDLE;
                VkImage sceneOffscreenCopySourceImage = VK_NULL_HANDLE;
                VkExtent2D sceneOffscreenCopySourceExtent = { 0, 0 };
                std::map<VkImageView, std::pair<VkImage, VkExtent2D>> frameOffscreenViewLookup;
                std::map<std::string, std::pair<VkImage, VkExtent2D>> namedRenderTargetImagesInFrame;
                auto getSampledImageLayoutForView = [&](VkImageView imageView) -> VkImageLayout
                {
                    if (imageView == VK_NULL_HANDLE)
                    {
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    auto sourceViewSearch = frameOffscreenViewLookup.find(imageView);
                    if (sourceViewSearch == std::end(frameOffscreenViewLookup))
                    {
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    auto layoutSearch = offscreenImageLayouts.find(sourceViewSearch->second.first);
                    if (layoutSearch == std::end(offscreenImageLayouts))
                    {
                        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }

                    const VkImageLayout trackedLayout = layoutSearch->second;
                    if (trackedLayout == VK_IMAGE_LAYOUT_GENERAL)
                    {
                        return VK_IMAGE_LAYOUT_GENERAL;
                    }

                    // Sampled-image descriptors should use shader-readable layouts.
                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                };

                auto updateSceneFallbackSourceFromNamedTarget = [&](const char *name) -> bool
                {
                    auto namedSearch = namedRenderTargetImagesInFrame.find(name);
                    if (namedSearch == std::end(namedRenderTargetImagesInFrame))
                    {
                        return false;
                    }

                    if (namedSearch->second.first == VK_NULL_HANDLE || namedSearch->second.second.width == 0 || namedSearch->second.second.height == 0)
                    {
                        return false;
                    }

                    sceneOffscreenCopySourceImage = namedSearch->second.first;
                    sceneOffscreenCopySourceExtent = namedSearch->second.second;
                    return true;
                };

                auto choosePreferredSceneFallbackSource = [&]()
                {
                    if (updateSceneFallbackSourceFromNamedTarget("alternateBuffer"))
                    {
                        return;
                    }

                    if (updateSceneFallbackSourceFromNamedTarget("finalBuffer"))
                    {
                        return;
                    }
                };

                constexpr bool allowSceneFallbackCopy = false;
                auto performSceneFallbackCopy = [&](bool transitionBackToColorAttachment)
                {
                    if (!allowSceneFallbackCopy ||
                        sceneOffscreenCopySourceImage == VK_NULL_HANDLE)
                    {
                        return;
                    }

                    auto sourceLayoutSearch = offscreenImageLayouts.find(sceneOffscreenCopySourceImage);
                    VkImageLayout sourceLayout = (sourceLayoutSearch != std::end(offscreenImageLayouts))
                        ? sourceLayoutSearch->second
                        : VK_IMAGE_LAYOUT_UNDEFINED;

                    if (sourceLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                    {
                        VkImageMemoryBarrier sourceToTransferSrc{};
                        sourceToTransferSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        sourceToTransferSrc.oldLayout = sourceLayout;
                        sourceToTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                        sourceToTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        sourceToTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        sourceToTransferSrc.image = sceneOffscreenCopySourceImage;
                        sourceToTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                        sourceToTransferSrc.subresourceRange.baseMipLevel = 0;
                        sourceToTransferSrc.subresourceRange.levelCount = 1;
                        sourceToTransferSrc.subresourceRange.baseArrayLayer = 0;
                        sourceToTransferSrc.subresourceRange.layerCount = 1;
                        sourceToTransferSrc.srcAccessMask = (sourceLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                            ? (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                            : VK_ACCESS_SHADER_READ_BIT;
                        sourceToTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                        VkPipelineStageFlags sourceStage = (sourceLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                            ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                            : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                        vkCmdPipelineBarrier(
                            commandBuffer,
                            sourceStage,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &sourceToTransferSrc);

                        offscreenImageLayouts[sceneOffscreenCopySourceImage] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    }

                    transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

                    VkImageBlit blitRegion{};
                    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blitRegion.srcSubresource.mipLevel = 0;
                    blitRegion.srcSubresource.baseArrayLayer = 0;
                    blitRegion.srcSubresource.layerCount = 1;
                    blitRegion.srcOffsets[0] = { 0, 0, 0 };
                    blitRegion.srcOffsets[1] = {
                        static_cast<int32_t>(std::max(sceneOffscreenCopySourceExtent.width, 1u)),
                        static_cast<int32_t>(std::max(sceneOffscreenCopySourceExtent.height, 1u)),
                        1 };

                    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    blitRegion.dstSubresource.mipLevel = 0;
                    blitRegion.dstSubresource.baseArrayLayer = 0;
                    blitRegion.dstSubresource.layerCount = 1;
                    blitRegion.dstOffsets[0] = { 0, 0, 0 };
                    blitRegion.dstOffsets[1] = {
                        static_cast<int32_t>(std::max(swapChainExtent.width, 1u)),
                        static_cast<int32_t>(std::max(swapChainExtent.height, 1u)),
                        1 };

                    vkCmdBlitImage(
                        commandBuffer,
                        sceneOffscreenCopySourceImage,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        swapChainImages[imageIndex],
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1,
                        &blitRegion,
                        VK_FILTER_LINEAR);

                    VkImageMemoryBarrier sourceToShaderRead{};
                    sourceToShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    sourceToShaderRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                    sourceToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    sourceToShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    sourceToShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    sourceToShaderRead.image = sceneOffscreenCopySourceImage;
                    sourceToShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    sourceToShaderRead.subresourceRange.baseMipLevel = 0;
                    sourceToShaderRead.subresourceRange.levelCount = 1;
                    sourceToShaderRead.subresourceRange.baseArrayLayer = 0;
                    sourceToShaderRead.subresourceRange.layerCount = 1;
                    sourceToShaderRead.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    sourceToShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    vkCmdPipelineBarrier(
                        commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0, nullptr,
                        0, nullptr,
                        1, &sourceToShaderRead);

                    offscreenImageLayouts[sceneOffscreenCopySourceImage] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                    if (transitionBackToColorAttachment)
                    {
                        transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
                    }
                };

                if (hasDrawCommands)
                {
                    transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

                    for (const auto &drawCommand : pendingDrawCommands)
                    {
                        if (drawCommand.commandType == DrawCommand::Type::ClearRenderTarget)
                        {
                            auto *targetTexture = drawCommand.clearRenderTarget;
                            if (!targetTexture || targetTexture->image == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            VkImage image = targetTexture->image;
                            VkImageLayout oldLayout = targetTexture->currentLayout;
                            auto oldLayoutSearch = offscreenImageLayouts.find(image);
                            if (oldLayoutSearch != std::end(offscreenImageLayouts))
                            {
                                oldLayout = oldLayoutSearch->second;
                            }

                            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                            {
                                oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            }

                            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            VkAccessFlags sourceAccess = 0;
                            switch (oldLayout)
                            {
                            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                sourceAccess = VK_ACCESS_TRANSFER_READ_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                sourceAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_GENERAL:
                                sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                sourceAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                sourceAccess = VK_ACCESS_SHADER_READ_BIT;
                                break;
                            default:
                                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                sourceAccess = 0;
                                break;
                            }

                            if (oldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                            {
                                VkImageMemoryBarrier toTransferDst{};
                                toTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                toTransferDst.oldLayout = oldLayout;
                                toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toTransferDst.image = image;
                                toTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                toTransferDst.subresourceRange.baseMipLevel = 0;
                                toTransferDst.subresourceRange.levelCount = 1;
                                toTransferDst.subresourceRange.baseArrayLayer = 0;
                                toTransferDst.subresourceRange.layerCount = 1;
                                toTransferDst.srcAccessMask = sourceAccess;
                                toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    sourceStage,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    1, &toTransferDst);
                            }

                            VkImageSubresourceRange clearRange{};
                            clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            clearRange.baseMipLevel = 0;
                            clearRange.levelCount = 1;
                            clearRange.baseArrayLayer = 0;
                            clearRange.layerCount = 1;
                            vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &drawCommand.clearRenderTargetColor, 1, &clearRange);

                            targetTexture->currentLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            offscreenImageLayouts[image] = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            continue;
                        }

                        if (drawCommand.commandType == DrawCommand::Type::ClearDepthStencil)
                        {
                            auto *depthTexture = drawCommand.clearDepthTarget;
                            if (!depthTexture || depthTexture->image == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            VkImageAspectFlags imageAspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                            const bool hasStencilAspect =
                                (depthTexture->format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
                                (depthTexture->format == VK_FORMAT_D24_UNORM_S8_UINT);
                            if (hasStencilAspect)
                            {
                                imageAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                            }

                            VkImageAspectFlags clearAspectMask = 0;
                            if ((drawCommand.clearDepthStencilFlags & Render::ClearFlags::Depth) != 0)
                            {
                                clearAspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
                            }
                            if (hasStencilAspect && ((drawCommand.clearDepthStencilFlags & Render::ClearFlags::Stencil) != 0))
                            {
                                clearAspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
                            }

                            if (clearAspectMask == 0)
                            {
                                continue;
                            }

                            VkImageLayout oldLayout = depthTexture->currentLayout;
                            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                            {
                                oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                            }

                            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            VkAccessFlags sourceAccess = 0;
                            switch (oldLayout)
                            {
                            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                sourceAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                sourceAccess = VK_ACCESS_TRANSFER_READ_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                sourceAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                sourceAccess = VK_ACCESS_SHADER_READ_BIT;
                                break;
                            case VK_IMAGE_LAYOUT_GENERAL:
                                sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                sourceAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                break;
                            default:
                                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                sourceAccess = 0;
                                break;
                            }

                            if (oldLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                            {
                                VkImageMemoryBarrier toTransferDst{};
                                toTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                toTransferDst.oldLayout = oldLayout;
                                toTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                toTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toTransferDst.image = depthTexture->image;
                                toTransferDst.subresourceRange.aspectMask = imageAspectMask;
                                toTransferDst.subresourceRange.baseMipLevel = 0;
                                toTransferDst.subresourceRange.levelCount = 1;
                                toTransferDst.subresourceRange.baseArrayLayer = 0;
                                toTransferDst.subresourceRange.layerCount = 1;
                                toTransferDst.srcAccessMask = sourceAccess;
                                toTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    sourceStage,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    1, &toTransferDst);
                            }

                            VkImageSubresourceRange clearRange{};
                            clearRange.aspectMask = clearAspectMask;
                            clearRange.baseMipLevel = 0;
                            clearRange.levelCount = 1;
                            clearRange.baseArrayLayer = 0;
                            clearRange.layerCount = 1;

                            VkClearDepthStencilValue clearValue{};
                            clearValue.depth = drawCommand.clearDepthValue;
                            clearValue.stencil = drawCommand.clearStencilValue;
                            vkCmdClearDepthStencilImage(commandBuffer, depthTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &clearRange);

                            VkImageMemoryBarrier toDepthAttachment{};
                            toDepthAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            toDepthAttachment.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            toDepthAttachment.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                            toDepthAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            toDepthAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            toDepthAttachment.image = depthTexture->image;
                            toDepthAttachment.subresourceRange.aspectMask = imageAspectMask;
                            toDepthAttachment.subresourceRange.baseMipLevel = 0;
                            toDepthAttachment.subresourceRange.levelCount = 1;
                            toDepthAttachment.subresourceRange.baseArrayLayer = 0;
                            toDepthAttachment.subresourceRange.layerCount = 1;
                            toDepthAttachment.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                            toDepthAttachment.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                            vkCmdPipelineBarrier(
                                commandBuffer,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                1, &toDepthAttachment);

                            depthTexture->currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                            continue;
                        }

                        if (drawCommand.commandType == DrawCommand::Type::CopyResource)
                        {
                            bool usedNamedSourceForCopy = false;
                            bool usedNamedDestinationForCopy = false;
                            auto *destinationBuffer = getObject<Buffer>(drawCommand.copyDestination);
                            auto *sourceBuffer = getObject<Buffer>(drawCommand.copySource);
                            if (destinationBuffer && sourceBuffer)
                            {
                                const VkDeviceSize copySize = std::min(destinationBuffer->size, sourceBuffer->size);
                                if (copySize > 0 && destinationBuffer->buffer != VK_NULL_HANDLE && sourceBuffer->buffer != VK_NULL_HANDLE)
                                {
                                    VkBufferCopy bufferCopy{};
                                    bufferCopy.srcOffset = 0;
                                    bufferCopy.dstOffset = 0;
                                    bufferCopy.size = copySize;
                                    vkCmdCopyBuffer(commandBuffer, sourceBuffer->buffer, destinationBuffer->buffer, 1, &bufferCopy);

                                    VkMemoryBarrier bufferCopyBarrier{};
                                    bufferCopyBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                                    bufferCopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                    bufferCopyBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                    vkCmdPipelineBarrier(
                                        commandBuffer,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        0,
                                        1, &bufferCopyBarrier,
                                        0, nullptr,
                                        0, nullptr);
                                }

                                continue;
                            }

                            auto *destinationTargetTexture = getObject<TargetTexture>(drawCommand.copyDestination);
                            auto *sourceTargetTexture = getObject<TargetTexture>(drawCommand.copySource);
                            auto *destinationViewTexture = getObject<ViewTexture>(drawCommand.copyDestination);
                            auto *sourceViewTexture = getObject<ViewTexture>(drawCommand.copySource);
                            auto *destinationDepthTexture = getObject<DepthTexture>(drawCommand.copyDestination);
                            auto *sourceDepthTexture = getObject<DepthTexture>(drawCommand.copySource);

                            VkImage destinationImage = VK_NULL_HANDLE;
                            VkImage sourceImage = VK_NULL_HANDLE;
                            VkImageLayout destinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            VkImageLayout sourceLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            uint32_t destinationWidth = 1;
                            uint32_t destinationHeight = 1;
                            uint32_t sourceWidth = 1;
                            uint32_t sourceHeight = 1;

                            if (destinationTargetTexture)
                            {
                                destinationImage = destinationTargetTexture->image;
                                destinationLayout = destinationTargetTexture->currentLayout;
                                destinationWidth = std::max(destinationTargetTexture->getDescription().width, 1u);
                                destinationHeight = std::max(destinationTargetTexture->getDescription().height, 1u);
                                auto destinationLayoutSearch = offscreenImageLayouts.find(destinationImage);
                                if (destinationLayoutSearch != std::end(offscreenImageLayouts))
                                {
                                    destinationLayout = destinationLayoutSearch->second;
                                }
                            }
                            else if (destinationViewTexture)
                            {
                                destinationImage = destinationViewTexture->image;
                                destinationWidth = std::max(destinationViewTexture->getDescription().width, 1u);
                                destinationHeight = std::max(destinationViewTexture->getDescription().height, 1u);
                            }
                            else if (destinationDepthTexture)
                            {
                                destinationImage = destinationDepthTexture->image;
                                destinationLayout = destinationDepthTexture->currentLayout;
                                destinationWidth = std::max(destinationDepthTexture->getDescription().width, 1u);
                                destinationHeight = std::max(destinationDepthTexture->getDescription().height, 1u);
                                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                            }

                            if (sourceTargetTexture)
                            {
                                sourceImage = sourceTargetTexture->image;
                                sourceLayout = sourceTargetTexture->currentLayout;
                                sourceWidth = std::max(sourceTargetTexture->getDescription().width, 1u);
                                sourceHeight = std::max(sourceTargetTexture->getDescription().height, 1u);
                                auto sourceLayoutSearch = offscreenImageLayouts.find(sourceImage);
                                if (sourceLayoutSearch != std::end(offscreenImageLayouts))
                                {
                                    sourceLayout = sourceLayoutSearch->second;
                                }
                            }
                            else if (sourceViewTexture)
                            {
                                sourceImage = sourceViewTexture->image;
                                sourceWidth = std::max(sourceViewTexture->getDescription().width, 1u);
                                sourceHeight = std::max(sourceViewTexture->getDescription().height, 1u);
                            }
                            else if (sourceDepthTexture)
                            {
                                sourceImage = sourceDepthTexture->image;
                                sourceLayout = sourceDepthTexture->currentLayout;
                                sourceWidth = std::max(sourceDepthTexture->getDescription().width, 1u);
                                sourceHeight = std::max(sourceDepthTexture->getDescription().height, 1u);
                                aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                            }

                            if (destinationImage == VK_NULL_HANDLE || sourceImage == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            if (destinationTargetTexture && destinationTargetTexture->imageView != VK_NULL_HANDLE)
                            {
                                frameOffscreenViewLookup[destinationTargetTexture->imageView] =
                                    std::make_pair(destinationImage, VkExtent2D{ std::max(destinationWidth, 1u), std::max(destinationHeight, 1u) });
                            }
                            if (sourceTargetTexture && sourceTargetTexture->imageView != VK_NULL_HANDLE)
                            {
                                frameOffscreenViewLookup[sourceTargetTexture->imageView] =
                                    std::make_pair(sourceImage, VkExtent2D{ std::max(sourceWidth, 1u), std::max(sourceHeight, 1u) });
                            }

                            const VkImage originalDestinationImage = destinationImage;
                            const VkImage originalSourceImage = sourceImage;
                            const VkImageLayout originalDestinationLayout = destinationLayout;
                            const VkImageLayout originalSourceLayout = sourceLayout;
                            const uint32_t originalDestinationWidth = destinationWidth;
                            const uint32_t originalDestinationHeight = destinationHeight;
                            const uint32_t originalSourceWidth = sourceWidth;
                            const uint32_t originalSourceHeight = sourceHeight;
                            if (destinationTargetTexture)
                            {
                                const auto destinationName = destinationTargetTexture->getDescription().name;
                                if (!destinationName.empty())
                                {
                                    auto namedDestinationSearch = namedRenderTargetImagesInFrame.find(destinationName);
                                    if (namedDestinationSearch != std::end(namedRenderTargetImagesInFrame) &&
                                        namedDestinationSearch->second.first != VK_NULL_HANDLE)
                                    {
                                        usedNamedDestinationForCopy = true;
                                        destinationImage = namedDestinationSearch->second.first;
                                        destinationWidth = std::max(namedDestinationSearch->second.second.width, 1u);
                                        destinationHeight = std::max(namedDestinationSearch->second.second.height, 1u);

                                        auto destinationLayoutSearch = offscreenImageLayouts.find(destinationImage);
                                        if (destinationLayoutSearch != std::end(offscreenImageLayouts))
                                        {
                                            destinationLayout = destinationLayoutSearch->second;
                                        }
                                    }
                                }
                            }

                            if (sourceTargetTexture)
                            {
                                const auto sourceName = sourceTargetTexture->getDescription().name;
                                if (!sourceName.empty())
                                {
                                    auto namedSourceSearch = namedRenderTargetImagesInFrame.find(sourceName);
                                    if (namedSourceSearch != std::end(namedRenderTargetImagesInFrame) &&
                                        namedSourceSearch->second.first != VK_NULL_HANDLE)
                                    {
                                        usedNamedSourceForCopy = true;
                                        sourceImage = namedSourceSearch->second.first;
                                        sourceWidth = std::max(namedSourceSearch->second.second.width, 1u);
                                        sourceHeight = std::max(namedSourceSearch->second.second.height, 1u);

                                        auto sourceLayoutSearch = offscreenImageLayouts.find(sourceImage);
                                        if (sourceLayoutSearch != std::end(offscreenImageLayouts))
                                        {
                                            sourceLayout = sourceLayoutSearch->second;
                                        }
                                    }
                                }
                            }

                            if (destinationImage == VK_NULL_HANDLE || sourceImage == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            if (destinationImage == sourceImage)
                            {
                                if (originalDestinationImage != VK_NULL_HANDLE &&
                                    originalSourceImage != VK_NULL_HANDLE &&
                                    originalDestinationImage != originalSourceImage)
                                {
                                    destinationImage = originalDestinationImage;
                                    sourceImage = originalSourceImage;
                                    destinationLayout = originalDestinationLayout;
                                    sourceLayout = originalSourceLayout;
                                    destinationWidth = originalDestinationWidth;
                                    destinationHeight = originalDestinationHeight;
                                    sourceWidth = originalSourceWidth;
                                    sourceHeight = originalSourceHeight;
                                    usedNamedDestinationForCopy = false;
                                    usedNamedSourceForCopy = false;
                                }
                            }

                            if (destinationImage == sourceImage)
                            {
                                continue;
                            }

                            auto getTransitionState = [](VkImageLayout layout, VkPipelineStageFlags &stage, VkAccessFlags &access)
                            {
                                switch (layout)
                                {
                                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                    access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    access = VK_ACCESS_TRANSFER_WRITE_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    access = VK_ACCESS_TRANSFER_READ_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_GENERAL:
                                    stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                    access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    access = VK_ACCESS_SHADER_READ_BIT;
                                    break;
                                case VK_IMAGE_LAYOUT_UNDEFINED:
                                default:
                                    stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                    access = 0;
                                    break;
                                }
                            };

                            VkPipelineStageFlags sourceSourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            VkAccessFlags sourceSourceAccess = 0;
                            getTransitionState(sourceLayout, sourceSourceStage, sourceSourceAccess);

                            VkPipelineStageFlags destinationSourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            VkAccessFlags destinationSourceAccess = 0;
                            getTransitionState(destinationLayout, destinationSourceStage, destinationSourceAccess);

                            const uint32_t destinationMipLevels = std::max<uint32_t>(
                                (destinationTargetTexture ? destinationTargetTexture->getDescription().mipMapCount :
                                    (destinationViewTexture ? destinationViewTexture->getDescription().mipMapCount :
                                        (destinationDepthTexture ? destinationDepthTexture->getDescription().mipMapCount : 1u))),
                                1u);
                            const uint32_t sourceMipLevels = std::max<uint32_t>(
                                (sourceTargetTexture ? sourceTargetTexture->getDescription().mipMapCount :
                                    (sourceViewTexture ? sourceViewTexture->getDescription().mipMapCount :
                                        (sourceDepthTexture ? sourceDepthTexture->getDescription().mipMapCount : 1u))),
                                1u);

                            std::array<VkImageMemoryBarrier, 2> toTransferBarriers{};
                            uint32_t toTransferBarrierCount = 0;

                            if (sourceLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                            {
                                auto &sourceToTransfer = toTransferBarriers[toTransferBarrierCount++];
                                sourceToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                sourceToTransfer.oldLayout = sourceLayout;
                                sourceToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                sourceToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                sourceToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                sourceToTransfer.image = sourceImage;
                                sourceToTransfer.subresourceRange.aspectMask = aspectMask;
                                sourceToTransfer.subresourceRange.baseMipLevel = 0;
                                sourceToTransfer.subresourceRange.levelCount = sourceMipLevels;
                                sourceToTransfer.subresourceRange.baseArrayLayer = 0;
                                sourceToTransfer.subresourceRange.layerCount = 1;
                                sourceToTransfer.srcAccessMask = sourceSourceAccess;
                                sourceToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                            }

                            if (destinationLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                            {
                                auto &destinationToTransfer = toTransferBarriers[toTransferBarrierCount++];
                                destinationToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                destinationToTransfer.oldLayout = destinationLayout;
                                destinationToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                destinationToTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                destinationToTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                destinationToTransfer.image = destinationImage;
                                destinationToTransfer.subresourceRange.aspectMask = aspectMask;
                                destinationToTransfer.subresourceRange.baseMipLevel = 0;
                                destinationToTransfer.subresourceRange.levelCount = destinationMipLevels;
                                destinationToTransfer.subresourceRange.baseArrayLayer = 0;
                                destinationToTransfer.subresourceRange.layerCount = 1;
                                destinationToTransfer.srcAccessMask = destinationSourceAccess;
                                destinationToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                            }

                            if (toTransferBarrierCount > 0)
                            {
                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    sourceSourceStage | destinationSourceStage,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    toTransferBarrierCount,
                                    toTransferBarriers.data());
                            }

                            VkImageCopy imageCopy{};
                            imageCopy.srcSubresource.aspectMask = aspectMask;
                            imageCopy.srcSubresource.mipLevel = 0;
                            imageCopy.srcSubresource.baseArrayLayer = 0;
                            imageCopy.srcSubresource.layerCount = 1;
                            imageCopy.srcOffset = { 0, 0, 0 };
                            imageCopy.dstSubresource.aspectMask = aspectMask;
                            imageCopy.dstSubresource.mipLevel = 0;
                            imageCopy.dstSubresource.baseArrayLayer = 0;
                            imageCopy.dstSubresource.layerCount = 1;
                            imageCopy.dstOffset = { 0, 0, 0 };
                            imageCopy.extent.width = std::min(sourceWidth, destinationWidth);
                            imageCopy.extent.height = std::min(sourceHeight, destinationHeight);
                            imageCopy.extent.depth = 1;

                            vkCmdCopyImage(
                                commandBuffer,
                                sourceImage,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                destinationImage,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                1,
                                &imageCopy);

                            std::array<VkImageMemoryBarrier, 2> fromTransferBarriers{};
                            uint32_t fromTransferBarrierCount = 0;

                            const VkImageLayout finalSourceLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            const VkImageLayout finalDestinationLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                            if (finalSourceLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                            {
                                auto &sourceFromTransfer = fromTransferBarriers[fromTransferBarrierCount++];
                                sourceFromTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                sourceFromTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                sourceFromTransfer.newLayout = finalSourceLayout;
                                sourceFromTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                sourceFromTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                sourceFromTransfer.image = sourceImage;
                                sourceFromTransfer.subresourceRange.aspectMask = aspectMask;
                                sourceFromTransfer.subresourceRange.baseMipLevel = 0;  
                                sourceFromTransfer.subresourceRange.levelCount = sourceMipLevels;
                                sourceFromTransfer.subresourceRange.baseArrayLayer = 0;
                                sourceFromTransfer.subresourceRange.layerCount = 1;
                                sourceFromTransfer.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                                sourceFromTransfer.dstAccessMask = (finalSourceLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                    ? VK_ACCESS_SHADER_READ_BIT
                                    : VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                            }

                            if (finalDestinationLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
                            {
                                auto &destinationFromTransfer = fromTransferBarriers[fromTransferBarrierCount++];
                                destinationFromTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                destinationFromTransfer.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                destinationFromTransfer.newLayout = finalDestinationLayout;
                                destinationFromTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                destinationFromTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                destinationFromTransfer.image = destinationImage;
                                destinationFromTransfer.subresourceRange.aspectMask = aspectMask;
                                destinationFromTransfer.subresourceRange.baseMipLevel = 0;
                                destinationFromTransfer.subresourceRange.levelCount = destinationMipLevels;
                                destinationFromTransfer.subresourceRange.baseArrayLayer = 0;
                                destinationFromTransfer.subresourceRange.layerCount = 1;
                                destinationFromTransfer.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                destinationFromTransfer.dstAccessMask = (finalDestinationLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                    ? VK_ACCESS_SHADER_READ_BIT
                                    : VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                            }

                            if (fromTransferBarrierCount > 0)
                            {
                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    fromTransferBarrierCount,
                                    fromTransferBarriers.data());
                            }

                            if (destinationTargetTexture)
                            {
                                destinationTargetTexture->currentLayout = finalDestinationLayout;
                                offscreenImageLayouts[destinationImage] = finalDestinationLayout;
                            }
                            if (sourceTargetTexture)
                            {
                                sourceTargetTexture->currentLayout = finalSourceLayout;
                                offscreenImageLayouts[sourceImage] = finalSourceLayout;
                            }
                            if (destinationDepthTexture)
                            {
                                destinationDepthTexture->currentLayout = finalDestinationLayout;
                            }
                            if (sourceDepthTexture)
                            {
                                sourceDepthTexture->currentLayout = finalSourceLayout;
                            }

                            continue;
                        }

                        if (drawCommand.commandType == DrawCommand::Type::GenerateMipMaps)
                        {
                            auto *targetTexture = getObject<TargetTexture>(drawCommand.mipmapTexture);
                            auto *viewTexture = getObject<ViewTexture>(drawCommand.mipmapTexture);
                            VkImage image = VK_NULL_HANDLE;
                            if (targetTexture)
                            {
                                image = targetTexture->image;
                            }
                            else if (viewTexture)
                            {
                                image = viewTexture->image;
                            }

                            if (targetTexture)
                            {
                                const auto &targetDescription = targetTexture->getDescription();
                                if (!targetDescription.name.empty())
                                {
                                    auto namedSearch = namedRenderTargetImagesInFrame.find(targetDescription.name);
                                    if (namedSearch != std::end(namedRenderTargetImagesInFrame) && namedSearch->second.first != VK_NULL_HANDLE)
                                    {
                                        image = namedSearch->second.first;
                                    }
                                }
                            }
                            else if (viewTexture)
                            {
                                const auto &viewDescription = viewTexture->getDescription();
                                if (!viewDescription.name.empty())
                                {
                                    auto namedSearch = namedRenderTargetImagesInFrame.find(viewDescription.name);
                                    if (namedSearch != std::end(namedRenderTargetImagesInFrame) && namedSearch->second.first != VK_NULL_HANDLE)
                                    {
                                        image = namedSearch->second.first;
                                    }
                                }
                            }

                            uint32_t textureWidth = 1;
                            uint32_t textureHeight = 1;
                            uint32_t textureMipCount = 0;
                            if (targetTexture)
                            {
                                const auto &description = targetTexture->getDescription();
                                textureWidth = std::max(description.width, 1u);
                                textureHeight = std::max(description.height, 1u);
                                textureMipCount = description.mipMapCount;
                            }
                            else if (viewTexture)
                            {
                                const auto &description = viewTexture->getDescription();
                                textureWidth = std::max(description.width, 1u);
                                textureHeight = std::max(description.height, 1u);
                                textureMipCount = description.mipMapCount;
                            }

                            uint32_t mipLevels = drawCommand.mipmapLevels;
                            if (mipLevels == 0)
                            {
                                mipLevels = textureMipCount;
                            }
                            if (mipLevels == 0)
                            {
                                const uint32_t maxDimension = std::max(textureWidth, textureHeight);
                                mipLevels = 1;
                                uint32_t currentDimension = maxDimension;
                                while (currentDimension > 1)
                                {
                                    currentDimension = std::max(currentDimension / 2, 1u);
                                    ++mipLevels;
                                }
                            }

                            if (image == VK_NULL_HANDLE || mipLevels <= 1)
                            {
                                continue;
                            }

                            VkImageLayout sourceLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            if (targetTexture)
                            {
                                auto layoutSearch = offscreenImageLayouts.find(image);
                                if (layoutSearch != std::end(offscreenImageLayouts))
                                {
                                    sourceLayout = layoutSearch->second;
                                }
                                else
                                {
                                    sourceLayout = targetTexture->currentLayout;
                                }
                            }

                            auto getLayoutTransition = [](VkImageLayout layout, VkPipelineStageFlags &stage, VkAccessFlags &access)
                            {
                                switch (layout)
                                {
                                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                    access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    access = VK_ACCESS_TRANSFER_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    access = VK_ACCESS_TRANSFER_READ_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_GENERAL:
                                    stage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                                    stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    access = VK_ACCESS_SHADER_READ_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_UNDEFINED:
                                default:
                                    stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                    access = 0;
                                    break;
                                }
                            };

                            VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                            VkAccessFlags sourceAccess = 0;
                            getLayoutTransition(sourceLayout, sourceStage, sourceAccess);

                            VkImageMemoryBarrier levelZeroToTransferSrc{};
                            levelZeroToTransferSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            levelZeroToTransferSrc.oldLayout = sourceLayout;
                            levelZeroToTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                            levelZeroToTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            levelZeroToTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            levelZeroToTransferSrc.image = image;
                            levelZeroToTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            levelZeroToTransferSrc.subresourceRange.baseMipLevel = 0;
                            levelZeroToTransferSrc.subresourceRange.levelCount = 1;
                            levelZeroToTransferSrc.subresourceRange.baseArrayLayer = 0;
                            levelZeroToTransferSrc.subresourceRange.layerCount = 1;
                            levelZeroToTransferSrc.srcAccessMask = sourceAccess;
                            levelZeroToTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                            vkCmdPipelineBarrier(
                                commandBuffer,
                                sourceStage,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                1, &levelZeroToTransferSrc);

                            VkImageMemoryBarrier remainingToTransferDst{};
                            remainingToTransferDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            remainingToTransferDst.oldLayout = sourceLayout;
                            remainingToTransferDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                            remainingToTransferDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            remainingToTransferDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            remainingToTransferDst.image = image;
                            remainingToTransferDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                            remainingToTransferDst.subresourceRange.baseMipLevel = 1;
                            remainingToTransferDst.subresourceRange.levelCount = mipLevels - 1;
                            remainingToTransferDst.subresourceRange.baseArrayLayer = 0;
                            remainingToTransferDst.subresourceRange.layerCount = 1;
                            remainingToTransferDst.srcAccessMask = sourceAccess;
                            remainingToTransferDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                            vkCmdPipelineBarrier(
                                commandBuffer,
                                sourceStage,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                1, &remainingToTransferDst);

                            int32_t mipWidth = std::max<int32_t>(static_cast<int32_t>(textureWidth), 1);
                            int32_t mipHeight = std::max<int32_t>(static_cast<int32_t>(textureHeight), 1);

                            for (uint32_t mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
                            {
                                VkImageBlit blitRegion{};
                                blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                blitRegion.srcSubresource.mipLevel = mipLevel - 1;
                                blitRegion.srcSubresource.baseArrayLayer = 0;
                                blitRegion.srcSubresource.layerCount = 1;
                                blitRegion.srcOffsets[0] = { 0, 0, 0 };
                                blitRegion.srcOffsets[1] = { mipWidth, mipHeight, 1 };

                                const int32_t nextMipWidth = std::max<int32_t>(mipWidth / 2, 1);
                                const int32_t nextMipHeight = std::max<int32_t>(mipHeight / 2, 1);

                                blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                blitRegion.dstSubresource.mipLevel = mipLevel;
                                blitRegion.dstSubresource.baseArrayLayer = 0;
                                blitRegion.dstSubresource.layerCount = 1;
                                blitRegion.dstOffsets[0] = { 0, 0, 0 };
                                blitRegion.dstOffsets[1] = { nextMipWidth, nextMipHeight, 1 };

                                vkCmdBlitImage(
                                    commandBuffer,
                                    image,
                                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                    image,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    1,
                                    &blitRegion,
                                    VK_FILTER_LINEAR);

                                if (mipLevel + 1 < mipLevels)
                                {
                                    VkImageMemoryBarrier levelToTransferSrc{};
                                    levelToTransferSrc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                    levelToTransferSrc.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                                    levelToTransferSrc.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                    levelToTransferSrc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    levelToTransferSrc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    levelToTransferSrc.image = image;
                                    levelToTransferSrc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                    levelToTransferSrc.subresourceRange.baseMipLevel = mipLevel;
                                    levelToTransferSrc.subresourceRange.levelCount = 1;
                                    levelToTransferSrc.subresourceRange.baseArrayLayer = 0;
                                    levelToTransferSrc.subresourceRange.layerCount = 1;
                                    levelToTransferSrc.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                                    levelToTransferSrc.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                                    vkCmdPipelineBarrier(
                                        commandBuffer,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        0,
                                        0, nullptr,
                                        0, nullptr,
                                        1, &levelToTransferSrc);
                                }

                                mipWidth = nextMipWidth;
                                mipHeight = nextMipHeight;
                            }

                            std::vector<VkImageMemoryBarrier> toShaderReadBarriers;
                            toShaderReadBarriers.reserve(mipLevels);

                            for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
                            {
                                VkImageMemoryBarrier toShaderRead{};
                                toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                toShaderRead.oldLayout = (mipLevel == mipLevels - 1)
                                    ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                                    : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                                toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toShaderRead.image = image;
                                toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                toShaderRead.subresourceRange.baseMipLevel = mipLevel;
                                toShaderRead.subresourceRange.levelCount = 1;
                                toShaderRead.subresourceRange.baseArrayLayer = 0;
                                toShaderRead.subresourceRange.layerCount = 1;
                                toShaderRead.srcAccessMask = (mipLevel == mipLevels - 1)
                                    ? VK_ACCESS_TRANSFER_WRITE_BIT
                                    : VK_ACCESS_TRANSFER_READ_BIT;
                                toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                toShaderReadBarriers.push_back(toShaderRead);
                            }

                            vkCmdPipelineBarrier(
                                commandBuffer,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                static_cast<uint32_t>(toShaderReadBarriers.size()),
                                toShaderReadBarriers.data());

                            if (targetTexture)
                            {
                                targetTexture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                offscreenImageLayouts[image] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            }

                            continue;
                        }

                        if (drawCommand.commandType == DrawCommand::Type::ComputeDispatch)
                        {
                            if (!drawCommand.computeProgram)
                            {
                                continue;
                            }

                            VkPipeline computePipeline = getOrCreateComputePipeline(drawCommand.computeProgram);
                            if (computePipeline == VK_NULL_HANDLE || descriptorSetLayout == VK_NULL_HANDLE || descriptorPool == VK_NULL_HANDLE || graphicsPipelineLayout == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            auto transitionComputeImageResource = [&](Render::Object *resource, bool unorderedAccess)
                            {
                                VkImage image = VK_NULL_HANDLE;
                                VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                VkImageLayout newLayout = unorderedAccess ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                bool hasTrackedLayout = false;

                                if (auto *targetTexture = getObject<TargetTexture>(resource))
                                {
                                    image = targetTexture->image;
                                    auto layoutSearch = offscreenImageLayouts.find(image);
                                    if (layoutSearch != std::end(offscreenImageLayouts))
                                    {
                                        oldLayout = layoutSearch->second;
                                        hasTrackedLayout = true;
                                    }
                                    else
                                    {
                                        oldLayout = targetTexture->currentLayout;
                                    }
                                }
                                else if (auto *depthTexture = getObject<DepthTexture>(resource))
                                {
                                    image = depthTexture->image;
                                    oldLayout = depthTexture->currentLayout;
                                    aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                                }
                                else
                                {
                                    return;
                                }

                                if (image == VK_NULL_HANDLE)
                                {
                                    return;
                                }

                                if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
                                {
                                    oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                }

                                if (oldLayout == newLayout)
                                {
                                    return;
                                }

                                VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                VkAccessFlags sourceAccess = 0;
                                switch (oldLayout)
                                {
                                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                                    sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                    sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    sourceAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                                    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                                    sourceAccess = VK_ACCESS_TRANSFER_READ_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_GENERAL:
                                    sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    sourceAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                                    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                                    sourceAccess = VK_ACCESS_SHADER_READ_BIT;
                                    break;

                                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                                    sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                                    sourceAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                    break;

                                default:
                                    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                    sourceAccess = 0;
                                    break;
                                }

                                VkImageMemoryBarrier transition{};
                                transition.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                transition.oldLayout = oldLayout;
                                transition.newLayout = newLayout;
                                transition.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                transition.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                transition.image = image;
                                transition.subresourceRange.aspectMask = aspectMask;
                                transition.subresourceRange.baseMipLevel = 0;
                                transition.subresourceRange.levelCount = 1;
                                transition.subresourceRange.baseArrayLayer = 0;
                                transition.subresourceRange.layerCount = 1;
                                transition.srcAccessMask = sourceAccess;
                                transition.dstAccessMask = unorderedAccess
                                    ? (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)
                                    : VK_ACCESS_SHADER_READ_BIT;

                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    sourceStage,
                                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    1, &transition);

                                if (auto *targetTexture = getObject<TargetTexture>(resource))
                                {
                                    targetTexture->currentLayout = newLayout;
                                    offscreenImageLayouts[image] = newLayout;
                                }
                                else if (auto *depthTexture = getObject<DepthTexture>(resource))
                                {
                                    depthTexture->currentLayout = newLayout;
                                }
                            };

                            for (uint32_t resourceSlot = 0; resourceSlot < PixelResourceSlotCount; ++resourceSlot)
                            {
                                transitionComputeImageResource(drawCommand.computeResources[resourceSlot], false);
                                transitionComputeImageResource(drawCommand.computeUnorderedAccessResources[resourceSlot], true);
                            }

                            VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
                            descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                            descriptorAllocateInfo.descriptorPool = descriptorPool;
                            descriptorAllocateInfo.descriptorSetCount = 1;
                            descriptorAllocateInfo.pSetLayouts = &descriptorSetLayout;

                            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                            if (vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSet) != VK_SUCCESS)
                            {
                                continue;
                            }

                            descriptorSets.push_back(descriptorSet);

                            std::array<VkDescriptorImageInfo, PixelResourceSlotCount * 3> imageInfos{};
                            uint32_t imageInfoCount = 0;
                            std::array<VkDescriptorBufferInfo, PixelResourceSlotCount * 3> bufferInfos{};
                            uint32_t bufferInfoCount = 0;
                            std::vector<VkWriteDescriptorSet> writes;
                            writes.reserve(PixelResourceSlotCount * 6);

                            for (uint32_t resourceSlot = 0; resourceSlot < PixelResourceSlotCount; ++resourceSlot)
                            {
                                if (drawCommand.computeResourceBuffers[resourceSlot] && drawCommand.computeResourceBuffers[resourceSlot]->buffer != VK_NULL_HANDLE)
                                {
                                    auto &resourceBufferInfo = bufferInfos[bufferInfoCount++];
                                    resourceBufferInfo.buffer = drawCommand.computeResourceBuffers[resourceSlot]->buffer;
                                    resourceBufferInfo.offset = 0;
                                    resourceBufferInfo.range = drawCommand.computeResourceBuffers[resourceSlot]->size;

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorStorageBufferBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                    write.pBufferInfo = &resourceBufferInfo;
                                    writes.push_back(write);
                                }

                                if (drawCommand.computeResourceImageViews[resourceSlot] != VK_NULL_HANDLE)
                                {
                                    auto &resourceImageInfo = imageInfos[imageInfoCount++];
                                    resourceImageInfo.imageLayout = getSampledImageLayoutForView(drawCommand.computeResourceImageViews[resourceSlot]);
                                    resourceImageInfo.imageView = drawCommand.computeResourceImageViews[resourceSlot];

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorSampledImageBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                                    write.pImageInfo = &resourceImageInfo;
                                    writes.push_back(write);
                                }

                                if (drawCommand.computeResourceSamplers[resourceSlot] != VK_NULL_HANDLE)
                                {
                                    auto &samplerInfo = imageInfos[imageInfoCount++];
                                    samplerInfo.sampler = drawCommand.computeResourceSamplers[resourceSlot];

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorSamplerBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                                    write.pImageInfo = &samplerInfo;
                                    writes.push_back(write);
                                }

                                if (drawCommand.computeUnorderedAccessBuffers[resourceSlot] && drawCommand.computeUnorderedAccessBuffers[resourceSlot]->buffer != VK_NULL_HANDLE)
                                {
                                    auto &unorderedBufferInfo = bufferInfos[bufferInfoCount++];
                                    unorderedBufferInfo.buffer = drawCommand.computeUnorderedAccessBuffers[resourceSlot]->buffer;
                                    unorderedBufferInfo.offset = 0;
                                    unorderedBufferInfo.range = drawCommand.computeUnorderedAccessBuffers[resourceSlot]->size;

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorStorageBufferBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                    write.pBufferInfo = &unorderedBufferInfo;
                                    writes.push_back(write);
                                }

                                if (drawCommand.computeUnorderedAccessImageViews[resourceSlot] != VK_NULL_HANDLE)
                                {
                                    auto &unorderedImageInfo = imageInfos[imageInfoCount++];
                                    unorderedImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                                    unorderedImageInfo.imageView = drawCommand.computeUnorderedAccessImageViews[resourceSlot];

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorStorageImageBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                                    write.pImageInfo = &unorderedImageInfo;
                                    writes.push_back(write);
                                }

                                Buffer *computeConstantBuffer = drawCommand.computeConstantBuffers[resourceSlot];
                                const VkBuffer computeConstantVkBuffer = getCapturedVkBuffer(computeConstantBuffer, drawCommand.computeConstantBufferVersions[resourceSlot]);
                                if (computeConstantBuffer && computeConstantVkBuffer != VK_NULL_HANDLE)
                                {
                                    auto &constantBufferInfo = bufferInfos[bufferInfoCount++];
                                    constantBufferInfo.buffer = computeConstantVkBuffer;
                                    constantBufferInfo.offset = 0;
                                    constantBufferInfo.range = computeConstantBuffer->size;

                                    VkWriteDescriptorSet write{};
                                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                    write.dstSet = descriptorSet;
                                    write.dstBinding = DescriptorVertexUniformBufferBase + resourceSlot;
                                    write.descriptorCount = 1;
                                    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                    write.pBufferInfo = &constantBufferInfo;
                                    writes.push_back(write);
                                }
                            }

                            if (!writes.empty())
                            {
                                vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
                            }

                            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
                            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, graphicsPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
                            vkCmdDispatch(commandBuffer,
                                drawCommand.computeThreadGroupCountX,
                                drawCommand.computeThreadGroupCountY,
                                drawCommand.computeThreadGroupCountZ);

                            VkMemoryBarrier memoryBarrier{};
                            memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                            memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                            memoryBarrier.dstAccessMask =
                                VK_ACCESS_SHADER_READ_BIT |
                                VK_ACCESS_SHADER_WRITE_BIT |
                                VK_ACCESS_UNIFORM_READ_BIT |
                                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
                                VK_ACCESS_INDEX_READ_BIT;

                            vkCmdPipelineBarrier(
                                commandBuffer,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                                    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                0,
                                1, &memoryBarrier,
                                0, nullptr,
                                0, nullptr);

                            continue;
                        }

                        for (uint32_t resourceSlot = 0; resourceSlot < PixelResourceSlotCount; ++resourceSlot)
                        {
                            VkImageView imageView = drawCommand.pixelResourceImageViews[resourceSlot];
                            if (imageView == VK_NULL_HANDLE)
                            {
                                continue;
                            }
                        }

                        const bool drawToBackBuffer = !drawCommand.hasOffscreenTarget;
                        if (!drawToBackBuffer && drawCommand.renderTarget)
                        {
                            const auto &targetDescription = drawCommand.renderTarget->getDescription();
                            if (!targetDescription.name.empty())
                            {
                                VkImage namedTargetImage = drawCommand.renderTarget->image;
                                VkExtent2D namedTargetExtent =
                                {
                                    std::max(targetDescription.width, 1u),
                                    std::max(targetDescription.height, 1u),
                                };

                                if (drawCommand.offscreenTargetCount > 0)
                                {
                                    if (drawCommand.offscreenImages[0] != VK_NULL_HANDLE)
                                    {
                                        namedTargetImage = drawCommand.offscreenImages[0];
                                    }

                                    if (drawCommand.offscreenExtents[0].width > 0 && drawCommand.offscreenExtents[0].height > 0)
                                    {
                                        namedTargetExtent = drawCommand.offscreenExtents[0];
                                    }
                                }

                                namedRenderTargetImagesInFrame[targetDescription.name] =
                                {
                                    namedTargetImage,
                                    namedTargetExtent,
                                };
                            }
                        }

                        VkRenderPass activeRenderPass = renderPass;
                        VkFramebuffer activeFramebuffer = swapChainFramebuffers[imageIndex];
                        VkExtent2D activeExtent = swapChainExtent;
                        bool activeRenderPassHasDepth = true;
                        VkImageView activeDepthImageView = depthImageView;
                        DepthTexture *activeDepthTexture = nullptr;
                        uint32_t offscreenTargetCount = drawCommand.offscreenTargetCount;
                        if (offscreenTargetCount > 8)
                        {
                            offscreenTargetCount = 8;
                        }
                        std::array<VkImage, 8> offscreenImages = drawCommand.offscreenImages;
                        std::array<VkImageView, 8> offscreenImageViews = drawCommand.offscreenImageViews;
                        std::array<VkFormat, 8> offscreenFormats = drawCommand.offscreenFormats;
                        std::array<uint8_t, 8> transitionedToColorAttachment{};

                        if (!drawToBackBuffer)
                        {
                            if (offscreenTargetCount == 0)
                            {
                                continue;
                            }

                            std::vector<VkFormat> targetFormats;
                            targetFormats.reserve(offscreenTargetCount);
                            bool validTargets = true;
                            for (uint32_t targetIndex = 0; targetIndex < offscreenTargetCount; ++targetIndex)
                            {
                                if (offscreenImages[targetIndex] == VK_NULL_HANDLE || offscreenImageViews[targetIndex] == VK_NULL_HANDLE || offscreenFormats[targetIndex] == VK_FORMAT_UNDEFINED)
                                {
                                    validTargets = false;
                                    break;
                                }

                                targetFormats.push_back(offscreenFormats[targetIndex]);
                            }

                            if (!validTargets)
                            {
                                continue;
                            }

                            activeExtent = drawCommand.offscreenExtents[0];
                            if (activeExtent.width == 0 || activeExtent.height == 0)
                            {
                                activeExtent = swapChainExtent;
                            }

                            const bool commandRequestsDepth =
                                (drawCommand.depthState != nullptr) &&
                                drawCommand.depthState->getDescription().enable;
                            VkFormat offscreenDepthFormat = VK_FORMAT_UNDEFINED;
                            if (commandRequestsDepth)
                            {
                                if (drawCommand.depthTarget && drawCommand.depthTarget->imageView != VK_NULL_HANDLE)
                                {
                                    activeDepthTexture = drawCommand.depthTarget;
                                    activeDepthImageView = activeDepthTexture->imageView;
                                    offscreenDepthFormat = activeDepthTexture->format;
                                }
                                else if ((depthImageView != VK_NULL_HANDLE) &&
                                    (activeExtent.width == swapChainExtent.width) &&
                                    (activeExtent.height == swapChainExtent.height))
                                {
                                    activeDepthImageView = depthImageView;
                                    offscreenDepthFormat = depthFormat;
                                }
                            }

                            const bool needsDepth =
                                commandRequestsDepth &&
                                (activeDepthImageView != VK_NULL_HANDLE) &&
                                (offscreenDepthFormat != VK_FORMAT_UNDEFINED);
                            activeRenderPassHasDepth = needsDepth;
                            activeRenderPass = getOrCreateOffscreenRenderPass(targetFormats, needsDepth ? offscreenDepthFormat : VK_FORMAT_UNDEFINED);
                            if (activeRenderPass == VK_NULL_HANDLE)
                            {
                                continue;
                            }

                            if (offscreenTargetCount > 0)
                            {
                                sceneOffscreenCopySourceImage = offscreenImages[0];
                                sceneOffscreenCopySourceExtent = activeExtent;
                            }

                            for (uint32_t targetIndex = 0; targetIndex < offscreenTargetCount; ++targetIndex)
                            {
                                if (offscreenImageViews[targetIndex] == VK_NULL_HANDLE || offscreenImages[targetIndex] == VK_NULL_HANDLE)
                                {
                                    continue;
                                }

                                VkExtent2D targetExtent = drawCommand.offscreenExtents[targetIndex];
                                if (targetExtent.width == 0 || targetExtent.height == 0)
                                {
                                    targetExtent = activeExtent;
                                }

                                frameOffscreenViewLookup[offscreenImageViews[targetIndex]] = std::make_pair(offscreenImages[targetIndex], targetExtent);
                            }

                            for (uint32_t targetIndex = 0; targetIndex < offscreenTargetCount; ++targetIndex)
                            {
                                if (offscreenImages[targetIndex] == VK_NULL_HANDLE)
                                {
                                    continue;
                                }

                                auto trackedLayoutSearch = offscreenImageLayouts.find(offscreenImages[targetIndex]);
                                VkImageLayout trackedLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                                if (trackedLayoutSearch != std::end(offscreenImageLayouts))
                                {
                                    trackedLayout = trackedLayoutSearch->second;
                                }

                                VkImageMemoryBarrier toColorAttachmentTarget{};
                                toColorAttachmentTarget.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                toColorAttachmentTarget.oldLayout = trackedLayout;
                                toColorAttachmentTarget.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                toColorAttachmentTarget.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toColorAttachmentTarget.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                toColorAttachmentTarget.image = offscreenImages[targetIndex];
                                toColorAttachmentTarget.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                toColorAttachmentTarget.subresourceRange.baseMipLevel = 0;
                                toColorAttachmentTarget.subresourceRange.levelCount = 1;
                                toColorAttachmentTarget.subresourceRange.baseArrayLayer = 0;
                                toColorAttachmentTarget.subresourceRange.layerCount = 1;
                                VkPipelineStageFlags targetSourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                                if (toColorAttachmentTarget.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                {
                                    toColorAttachmentTarget.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                                    targetSourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                                }
                                else if (toColorAttachmentTarget.oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                                {
                                    toColorAttachmentTarget.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    targetSourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                                }
                                else
                                {
                                    toColorAttachmentTarget.srcAccessMask = 0;
                                }
                                toColorAttachmentTarget.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    targetSourceStage,
                                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    1, &toColorAttachmentTarget);

                                offscreenImageLayouts[offscreenImages[targetIndex]] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                transitionedToColorAttachment[targetIndex] = 1;
                            }

                            if (needsDepth && activeDepthTexture)
                            {
                                VkImageMemoryBarrier depthToAttachment{};
                                depthToAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                depthToAttachment.oldLayout = activeDepthTexture->currentLayout;
                                depthToAttachment.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                                depthToAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                depthToAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                depthToAttachment.image = activeDepthTexture->image;
                                depthToAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                                depthToAttachment.subresourceRange.baseMipLevel = 0;
                                depthToAttachment.subresourceRange.levelCount = 1;
                                depthToAttachment.subresourceRange.baseArrayLayer = 0;
                                depthToAttachment.subresourceRange.layerCount = 1;
                                depthToAttachment.srcAccessMask = (activeDepthTexture->currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                    ? VK_ACCESS_SHADER_READ_BIT
                                    : 0;
                                depthToAttachment.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                                vkCmdPipelineBarrier(
                                    commandBuffer,
                                    (activeDepthTexture->currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                        ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                                        : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                    0,
                                    0, nullptr,
                                    0, nullptr,
                                    1, &depthToAttachment);

                                activeDepthTexture->currentLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                            }

                            std::vector<VkImageView> framebufferAttachmentViews(offscreenImageViews.begin(), offscreenImageViews.begin() + offscreenTargetCount);
                            if (needsDepth && activeDepthImageView != VK_NULL_HANDLE)
                            {
                                framebufferAttachmentViews.push_back(activeDepthImageView);
                            }

                            activeFramebuffer = getOrCreateOffscreenFramebuffer(activeRenderPass, framebufferAttachmentViews, activeExtent);
                            if (activeFramebuffer == VK_NULL_HANDLE)
                            {
                                continue;
                            }
                        }

                        VkRenderPassBeginInfo renderPassBeginInfo{};
                        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassBeginInfo.renderPass = activeRenderPass;
                        renderPassBeginInfo.framebuffer = activeFramebuffer;
                        renderPassBeginInfo.renderArea.offset = { 0, 0 };
                        renderPassBeginInfo.renderArea.extent = activeExtent;

                        std::array<VkClearValue, 9> clearValues{};
                        uint32_t clearValueCount = 0;
                        if (drawToBackBuffer)
                        {
                            clearValues[clearValueCount++].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                            clearValues[clearValueCount++].depthStencil = { 1.0f, 0 };
                        }
                        else
                        {
                            for (uint32_t targetIndex = 0; targetIndex < offscreenTargetCount; ++targetIndex)
                            {
                                clearValues[clearValueCount++].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                            }

                            if (activeRenderPassHasDepth)
                            {
                                clearValues[clearValueCount++].depthStencil = { 1.0f, 0 };
                            }
                        }

                        renderPassBeginInfo.clearValueCount = clearValueCount;
                        renderPassBeginInfo.pClearValues = clearValues.data();

                        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                        auto endRenderPassForCurrentTarget = [&]()
                        {
                            vkCmdEndRenderPass(commandBuffer);

                            if (!drawToBackBuffer)
                            {
                                for (uint32_t targetIndex = 0; targetIndex < offscreenTargetCount; ++targetIndex)
                                {
                                    if (offscreenImages[targetIndex] == VK_NULL_HANDLE || transitionedToColorAttachment[targetIndex] == 0)
                                    {
                                        continue;
                                    }

                                    VkImageMemoryBarrier toShaderReadTarget{};
                                    toShaderReadTarget.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                    toShaderReadTarget.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                                    toShaderReadTarget.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    toShaderReadTarget.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    toShaderReadTarget.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    toShaderReadTarget.image = offscreenImages[targetIndex];
                                    toShaderReadTarget.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                                    toShaderReadTarget.subresourceRange.baseMipLevel = 0;
                                    toShaderReadTarget.subresourceRange.levelCount = 1;
                                    toShaderReadTarget.subresourceRange.baseArrayLayer = 0;
                                    toShaderReadTarget.subresourceRange.layerCount = 1;
                                    toShaderReadTarget.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                                    toShaderReadTarget.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                                    vkCmdPipelineBarrier(
                                        commandBuffer,
                                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                        0,
                                        0, nullptr,
                                        0, nullptr,
                                        1, &toShaderReadTarget);

                                    offscreenImageLayouts[offscreenImages[targetIndex]] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                }

                                if (activeDepthTexture)
                                {
                                    VkImageMemoryBarrier depthToShaderRead{};
                                    depthToShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                                    depthToShaderRead.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                                    depthToShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    depthToShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    depthToShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                                    depthToShaderRead.image = activeDepthTexture->image;
                                    depthToShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                                    depthToShaderRead.subresourceRange.baseMipLevel = 0;
                                    depthToShaderRead.subresourceRange.levelCount = 1;
                                    depthToShaderRead.subresourceRange.baseArrayLayer = 0;
                                    depthToShaderRead.subresourceRange.layerCount = 1;
                                    depthToShaderRead.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                                    depthToShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                                    vkCmdPipelineBarrier(
                                        commandBuffer,
                                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                        0,
                                        0, nullptr,
                                        0, nullptr,
                                        1, &depthToShaderRead);

                                    activeDepthTexture->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                }
                            }
                        };

                        const DrawCommand *pipelineCommand = &drawCommand;
                        DrawCommand backBufferCompositionCommand{};
                        if (drawToBackBuffer)
                        {
                            backBufferCompositionCommand = drawCommand;
                            backBufferCompositionCommand.depthState = nullptr;
                            backBufferCompositionCommand.renderState = nullptr;
                            pipelineCommand = &backBufferCompositionCommand;
                        }

                        VkPipeline pipeline = getOrCreateGraphicsPipeline(*pipelineCommand, activeRenderPass);
                        if (pipeline == VK_NULL_HANDLE)
                        {
                            endRenderPassForCurrentTarget();
                            continue;
                        }

                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                        VkViewport viewport = drawCommand.viewport;
                        if (viewport.width <= 1.0f || viewport.height <= 1.0f)
                        {
                            viewport.x = 0.0f;
                            viewport.y = 0.0f;
                            viewport.width = static_cast<float>(activeExtent.width);
                            viewport.height = static_cast<float>(activeExtent.height);
                            viewport.minDepth = 0.0f;
                            viewport.maxDepth = 1.0f;
                        }

                        // Vulkan and D3D-style clip spaces differ in Y orientation.
                        viewport.y = viewport.y + viewport.height;
                        viewport.height = -viewport.height;

                        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                        VkRect2D scissor = drawCommand.scissor;
                        if (scissor.extent.width == 0 || scissor.extent.height == 0)
                        {
                            scissor.offset = { 0, 0 };
                            scissor.extent = activeExtent;
                        }
                        else
                        {
                            const int32_t maxWidth = static_cast<int32_t>(activeExtent.width);
                            const int32_t maxHeight = static_cast<int32_t>(activeExtent.height);

                            int32_t left = std::clamp(scissor.offset.x, 0, maxWidth);
                            int32_t top = std::clamp(scissor.offset.y, 0, maxHeight);
                            int32_t right = std::clamp(scissor.offset.x + static_cast<int32_t>(scissor.extent.width), 0, maxWidth);
                            int32_t bottom = std::clamp(scissor.offset.y + static_cast<int32_t>(scissor.extent.height), 0, maxHeight);

                            if (right <= left || bottom <= top)
                            {
                                endRenderPassForCurrentTarget();
                                continue;
                            }

                            scissor.offset.x = left;
                            scissor.offset.y = top;
                            scissor.extent.width = static_cast<uint32_t>(right - left);
                            scissor.extent.height = static_cast<uint32_t>(bottom - top);
                        }
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                        for (uint32_t slot = 0; slot < drawCommand.vertexBuffers.size(); ++slot)
                        {
                            if (!drawCommand.vertexBuffers[slot])
                            {
                                continue;
                            }

                            VkBuffer vertexBuffer = getCapturedVkBuffer(drawCommand.vertexBuffers[slot], drawCommand.vertexBufferVersions[slot]);
                            if (vertexBuffer == VK_NULL_HANDLE)
                            {
                                continue;
                            }
                            VkDeviceSize vertexOffset = drawCommand.vertexOffsets[slot];
                            vkCmdBindVertexBuffers(commandBuffer, slot, 1, &vertexBuffer, &vertexOffset);
                        }

                        if (drawCommand.indexed)
                        {
                            if (!drawCommand.indexBuffer || drawCommand.indexCount == 0)
                            {
                                endRenderPassForCurrentTarget();
                                continue;
                            }

                            const VkIndexType indexType = (drawCommand.indexBuffer->getDescription().format == Render::Format::R16_UINT) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
                            VkBuffer indexBuffer = getCapturedVkBuffer(drawCommand.indexBuffer, drawCommand.indexBufferVersion);
                            if (indexBuffer == VK_NULL_HANDLE)
                            {
                                endRenderPassForCurrentTarget();
                                continue;
                            }
                            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, drawCommand.indexOffset, indexType);
                        }

                        if (drawToBackBuffer)
                        {
                            VkImageView sourceView = drawCommand.pixelResourceImageViews[0];
                            if (sourceView == VK_NULL_HANDLE)
                            {
                                sourceView = drawCommand.pixelImageView;
                            }

                            auto sourceViewSearch = frameOffscreenViewLookup.find(sourceView);
                            if (sourceViewSearch != std::end(frameOffscreenViewLookup))
                            {
                                sceneOffscreenCopySourceImage = sourceViewSearch->second.first;
                                sceneOffscreenCopySourceExtent = sourceViewSearch->second.second;
                            }
                        }

                        if (descriptorSetLayout != VK_NULL_HANDLE && descriptorPool != VK_NULL_HANDLE && graphicsPipelineLayout != VK_NULL_HANDLE)
                        {
                            GraphicsDescriptorSignature descriptorSignature{};
                            descriptorSignature.pixelResourceImageViews = drawCommand.pixelResourceImageViews;
                            descriptorSignature.pixelSamplerStates = drawCommand.pixelSamplerStates;
                            descriptorSignature.pixelResourceBuffers = drawCommand.pixelResourceBuffers;
                            descriptorSignature.pixelConstantBuffers = drawCommand.pixelConstantBuffers;
                            descriptorSignature.vertexConstantBuffers = drawCommand.vertexConstantBuffers;

                            if (false && hasLastGraphicsDescriptorSignature && 
                                lastGraphicsDescriptorSet != VK_NULL_HANDLE &&
                                (descriptorSignature == lastGraphicsDescriptorSignature))
                            {
                                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &lastGraphicsDescriptorSet, 0, nullptr);
                            }
                            else
                            {
                                VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
                                descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                                descriptorAllocateInfo.descriptorPool = descriptorPool;
                                descriptorAllocateInfo.descriptorSetCount = 1;
                                descriptorAllocateInfo.pSetLayouts = &descriptorSetLayout;

                                VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                                if (vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSet) == VK_SUCCESS)
                                {
                                    descriptorSets.push_back(descriptorSet);
                                    std::array<VkDescriptorImageInfo, PixelResourceSlotCount> sampledImageInfos{};
                                    uint32_t sampledImageInfoCount = 0;

                                    std::array<VkDescriptorImageInfo, PixelResourceSlotCount> samplerInfos{};
                                    uint32_t samplerInfoCount = 0;

                                    std::array<VkDescriptorBufferInfo, PixelResourceSlotCount * 3> bufferInfos{};
                                    uint32_t bufferInfoCount = 0;

                                    std::vector<VkWriteDescriptorSet> writes;
                                    writes.reserve((PixelResourceSlotCount * 4) + PixelResourceSlotCount);

                                    for (uint32_t resourceSlot = 0; resourceSlot < PixelResourceSlotCount; ++resourceSlot)
                                    {
                                        VkImageView imageView = drawCommand.pixelResourceImageViews[resourceSlot];
                                        VkSampler sampler = drawCommand.pixelSamplerStates[resourceSlot];
                                        Buffer *resourceBuffer = drawCommand.pixelResourceBuffers[resourceSlot];

                                        if (resourceBuffer && resourceBuffer->buffer != VK_NULL_HANDLE)
                                        {
                                            auto &resourceBufferInfo = bufferInfos[bufferInfoCount++];
                                            resourceBufferInfo.buffer = resourceBuffer->buffer;
                                            resourceBufferInfo.offset = 0;
                                            resourceBufferInfo.range = resourceBuffer->size;

                                            VkWriteDescriptorSet write{};
                                            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                            write.dstSet = descriptorSet;
                                            write.dstBinding = DescriptorStorageBufferBase + resourceSlot;
                                            write.descriptorCount = 1;
                                            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                                            write.pBufferInfo = &resourceBufferInfo;
                                            writes.push_back(write);

                                        }

                                        if (imageView != VK_NULL_HANDLE)
                                        {
                                            auto &sampledImageInfo = sampledImageInfos[sampledImageInfoCount++];
                                            sampledImageInfo.imageLayout = getSampledImageLayoutForView(imageView);
                                            sampledImageInfo.imageView = imageView;

                                            VkWriteDescriptorSet write{};
                                            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                            write.dstSet = descriptorSet;
                                            write.dstBinding = DescriptorSampledImageBase + resourceSlot;
                                            write.descriptorCount = 1;
                                            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                                            write.pImageInfo = &sampledImageInfo;
                                            writes.push_back(write);
                                        }

                                        if (sampler == VK_NULL_HANDLE)
                                        {
                                            sampler = drawCommand.pixelResourceSamplers[resourceSlot];
                                        }

                                        if (sampler != VK_NULL_HANDLE)
                                        {
                                            auto &samplerInfo = samplerInfos[samplerInfoCount++];
                                            samplerInfo.sampler = sampler;

                                            VkWriteDescriptorSet write{};
                                            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                            write.dstSet = descriptorSet;
                                            write.dstBinding = DescriptorSamplerBase + resourceSlot;
                                            write.descriptorCount = 1;
                                            write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                                            write.pImageInfo = &samplerInfo;
                                            writes.push_back(write);
                                        }
                                    }

                                    for (uint32_t constantStage = 0; constantStage < PixelResourceSlotCount; ++constantStage)
                                    {
                                        Buffer *pixelConstantBuffer = drawCommand.pixelConstantBuffers[constantStage];
                                        const VkBuffer pixelConstantVkBuffer = getCapturedVkBuffer(pixelConstantBuffer, drawCommand.pixelConstantBufferVersions[constantStage]);
                                        if (pixelConstantBuffer && pixelConstantVkBuffer != VK_NULL_HANDLE)
                                        {
                                            auto &pixelConstantBufferInfo = bufferInfos[bufferInfoCount++];
                                            pixelConstantBufferInfo.buffer = pixelConstantVkBuffer;
                                            pixelConstantBufferInfo.offset = 0;
                                            pixelConstantBufferInfo.range = pixelConstantBuffer->size;

                                            VkWriteDescriptorSet write{};
                                            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                            write.dstSet = descriptorSet;
                                            write.dstBinding = DescriptorPixelUniformBufferBase + constantStage;
                                            write.descriptorCount = 1;
                                            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                            write.pBufferInfo = &pixelConstantBufferInfo;
                                            writes.push_back(write);
                                        }

                                        Buffer *vertexConstantBuffer = drawCommand.vertexConstantBuffers[constantStage];
                                        const VkBuffer vertexConstantVkBuffer = getCapturedVkBuffer(vertexConstantBuffer, drawCommand.vertexConstantBufferVersions[constantStage]);
                                        if (!vertexConstantBuffer || vertexConstantVkBuffer == VK_NULL_HANDLE)
                                        {
                                            continue;
                                        }

                                        auto &vertexConstantBufferInfo = bufferInfos[bufferInfoCount++];
                                        vertexConstantBufferInfo.buffer = vertexConstantVkBuffer;
                                        vertexConstantBufferInfo.offset = 0;
                                        vertexConstantBufferInfo.range = vertexConstantBuffer->size;

                                        VkWriteDescriptorSet write{};
                                        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                        write.dstSet = descriptorSet;
                                        write.dstBinding = DescriptorVertexUniformBufferBase + constantStage;
                                        write.descriptorCount = 1;
                                        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                        write.pBufferInfo = &vertexConstantBufferInfo;
                                        writes.push_back(write);
                                    }

                                    if (!writes.empty())
                                    {
                                        vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
                                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
                                        lastGraphicsDescriptorSignature = descriptorSignature;
                                        hasLastGraphicsDescriptorSignature = true;
                                        lastGraphicsDescriptorSet = descriptorSet;
                                    }
                                }
                                else
                                {
                                    endRenderPassForCurrentTarget();
                                    continue;
                                }
                            }
                        }

                        if (drawCommand.indexed)
                        {
                            vkCmdDrawIndexed(commandBuffer, drawCommand.indexCount, std::max(drawCommand.instanceCount, 1u), 0, drawCommand.firstVertex, drawCommand.firstInstance);
                            if (drawToBackBuffer)
                            {
                                bool hasPixelImage = (drawCommand.pixelImageView != VK_NULL_HANDLE);
                                if (!hasPixelImage)
                                {
                                    hasPixelImage = std::any_of(
                                        std::begin(drawCommand.pixelResourceImageViews),
                                        std::end(drawCommand.pixelResourceImageViews),
                                        [](VkImageView imageView) { return imageView != VK_NULL_HANDLE; });
                                }
                            }
                        }
                        else if (drawCommand.vertexCount > 0)
                        {
                            vkCmdDraw(commandBuffer, drawCommand.vertexCount, std::max(drawCommand.instanceCount, 1u), static_cast<uint32_t>(drawCommand.firstVertex), drawCommand.firstInstance);
                            if (drawToBackBuffer)
                            {
                                bool hasPixelImage = (drawCommand.pixelImageView != VK_NULL_HANDLE);
                                if (!hasPixelImage)
                                {
                                    hasPixelImage   = std::any_of(
                                        std::begin(drawCommand.pixelResourceImageViews),
                                        std::end(drawCommand.pixelResourceImageViews),
                                        [](VkImageView imageView) { return imageView != VK_NULL_HANDLE; });
                                }
                            }
                        }

                        endRenderPassForCurrentTarget();
                    }
                }

                if (hasDrawCommands)
                {
                    choosePreferredSceneFallbackSource();
                    performSceneFallbackCopy(false);
                }

                transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
                if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan failed to record command buffer");
                    pendingDrawCommands.clear();
                    return;
                }

                const auto recordEndTime = std::chrono::high_resolution_clock::now();
                recordCpuMs = std::chrono::duration<double, std::milli>(recordEndTime - recordStartTime).count();

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT };
                VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = waitSemaphores;
                submitInfo.pWaitDstStageMask = waitStages;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = signalSemaphores;

                vkResetFences(device, 1, &inFlightFence);
                VkResult submitResult = VK_SUCCESS;
                double submitCpuMs = 0.0;
                {
                    const auto submitStartTime = std::chrono::high_resolution_clock::now();
                    std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                    submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
                    const auto submitEndTime = std::chrono::high_resolution_clock::now();
                    submitCpuMs = std::chrono::duration<double, std::milli>(submitEndTime - submitStartTime).count();
                }
                if (submitResult != VK_SUCCESS)
                {
                    if (submitResult == VK_ERROR_DEVICE_LOST)
                    {
                        handleDeviceLost(submitResult, "vkQueueSubmit");
                        return;
                    }

                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan failed to submit draw command buffer: result={}",
                        static_cast<int32_t>(submitResult));
                    pendingDrawCommands.clear();
                    recreateSwapChain();
                    return;
                }

                inFlightFencePending = true;

                VkPresentInfoKHR presentInfo{};
                presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                presentInfo.waitSemaphoreCount = 1;
                presentInfo.pWaitSemaphores = signalSemaphores;
                presentInfo.swapchainCount = 1;
                presentInfo.pSwapchains = &swapChain;
                presentInfo.pImageIndices = &imageIndex;

                VkResult presentResult = VK_SUCCESS;
                double presentCpuMs = 0.0;
                {
                    const auto presentStartTime = std::chrono::high_resolution_clock::now();
                    std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                    presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
                    const auto presentEndTime = std::chrono::high_resolution_clock::now();
                    presentCpuMs = std::chrono::duration<double, std::milli>(presentEndTime - presentStartTime).count();
                }
                if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
                {
                    recreateSwapChain();
                }
                else if (presentResult == VK_ERROR_DEVICE_LOST)
                {
                    handleDeviceLost(presentResult, "vkQueuePresentKHR");
                    return;
                }
                else if (presentResult != VK_SUCCESS)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan failed to present swap-chain image: result={}",
                        static_cast<int32_t>(presentResult));
                    pendingDrawCommands.clear();
                    recreateSwapChain();
                    return;
                }

                ++presentFrameIndex;
                const uint32_t totalCommandCount = static_cast<uint32_t>(pendingDrawCommands.size());
                getContext()->setRuntimeMetric("vulkan.frame", static_cast<double>(presentFrameIndex));
                getContext()->setRuntimeMetric("vulkan.totalCommands", static_cast<double>(totalCommandCount));
                getContext()->setRuntimeMetric("vulkan.constantBufferVersioningEnabled", (constantBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? 1.0 : 0.0);
                getContext()->setRuntimeMetric("vulkan.vertexBufferVersioningEnabled", (vertexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? 1.0 : 0.0);
                getContext()->setRuntimeMetric("vulkan.indexBufferVersioningEnabled", (indexBufferVersioningPolicy.mode == Render::BufferVersioningMode::FixedRing) ? 1.0 : 0.0);
                getContext()->setRuntimeMetric("vulkan.constantBufferRingSize", static_cast<double>(constantBufferVersioningPolicy.ringSize));
                getContext()->setRuntimeMetric("vulkan.vertexBufferRingSize", static_cast<double>(vertexBufferVersioningPolicy.ringSize));
                getContext()->setRuntimeMetric("vulkan.indexBufferRingSize", static_cast<double>(indexBufferVersioningPolicy.ringSize));
                getContext()->setRuntimeMetric("render.frame", static_cast<double>(presentFrameIndex));
                getContext()->setRuntimeMetric("render.backend", 0.0);
                getContext()->setRuntimeMetric("render.totalCommands", static_cast<double>(totalCommandCount));
                getContext()->setRuntimeMetric("render.mappedDepthFunc", 0.0);
                getContext()->setRuntimeMetric("render.firstIndexedInstanceCount", 0.0);
                getContext()->setRuntimeMetric("vulkan.submitCpuMs", submitCpuMs);
                getContext()->setRuntimeMetric("vulkan.presentCpuMs", presentCpuMs);
                getContext()->setRuntimeMetric("render.presentCpuMs", (submitCpuMs + presentCpuMs));
                const double frameCpuMs = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - frameCpuStartTime).count();
                const double knownFrameCpuMs = waitFenceCpuMs + acquireCpuMs + recordCpuMs + submitCpuMs + presentCpuMs + cleanupCpuMs + drawCommandLockCpuMs;
                const double untrackedFrameCpuMs = std::max(0.0, frameCpuMs - knownFrameCpuMs);

                getContext()->setRuntimeMetric("vulkan.waitFenceCpuMs", waitFenceCpuMs);
                getContext()->setRuntimeMetric("vulkan.acquireCpuMs", acquireCpuMs);
                getContext()->setRuntimeMetric("vulkan.recordCpuMs", recordCpuMs);
                getContext()->setRuntimeMetric("vulkan.cleanupCpuMs", cleanupCpuMs);
                getContext()->setRuntimeMetric("vulkan.cleanupFramebufferCpuMs", cleanupFramebufferCpuMs);
                getContext()->setRuntimeMetric("vulkan.cleanupDescriptorCpuMs", cleanupDescriptorCpuMs);
                getContext()->setRuntimeMetric("vulkan.drawCommandLockCpuMs", drawCommandLockCpuMs);
                getContext()->setRuntimeMetric("vulkan.untrackedFrameCpuMs", untrackedFrameCpuMs);
                getContext()->setRuntimeMetric("vulkan.frameCpuMs", frameCpuMs);
                getContext()->setRuntimeMetric("vulkan.deferredContextQueues", static_cast<double>(deferredContextDrawCommands.size()));
                getContext()->setRuntimeMetric("vulkan.deferredCommandLists", static_cast<double>(deferredCommandLists.size()));
                pendingDrawCommands.clear();
            }
        };

        void Device::enqueueGenerateMipMapsCommand(Context *sourceContext, Render::Texture *texture)
        {
            if (!sourceContext || !texture)
            {
                return;
            }

            Device::DrawCommand command;
            command.commandType = Device::DrawCommand::Type::GenerateMipMaps;
            command.mipmapTexture = texture;
            if (auto *targetTexture = getObject<TargetTexture>(texture))
            {
                command.mipmapLevels = targetTexture->supportsMipBlit
                    ? std::max(texture->getDescription().mipMapCount, 1u)
                    : 1u;
            }
            else
            {
                command.mipmapLevels = std::max(texture->getDescription().mipMapCount, 1u);
            }

            std::lock_guard<std::mutex> lock(getDrawCommandMutex());
            if (sourceContext->isDeferredContext)
            {
                deferredContextDrawCommands[sourceContext].push_back(command);
            }
            else
            {
                pendingDrawCommands.push_back(command);
            }
        }

        void Device::enqueueComputeDispatchCommand(Context *sourceContext, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
        {
            if (!sourceContext)
            {
                return;
            }

            Device::DrawCommand command;
            command.commandType = Device::DrawCommand::Type::ComputeDispatch;
            command.computeProgram = sourceContext->currentComputeProgram;
            command.computeThreadGroupCountX = threadGroupCountX;
            command.computeThreadGroupCountY = threadGroupCountY;
            command.computeThreadGroupCountZ = threadGroupCountZ;
            command.computeConstantBuffers = sourceContext->currentComputeConstantBuffers;
            command.computeResources = sourceContext->currentComputeResources;
            command.computeResourceImageViews = sourceContext->currentComputeResourceImageViews;
            command.computeResourceSamplers = sourceContext->currentComputeResourceSamplers;
            command.computeResourceBuffers = sourceContext->currentComputeResourceBuffers;
            command.computeUnorderedAccessResources = sourceContext->currentComputeUnorderedAccessResources;
            command.computeUnorderedAccessImageViews = sourceContext->currentComputeUnorderedAccessImageViews;
            command.computeUnorderedAccessBuffers = sourceContext->currentComputeUnorderedAccessBuffers;

            std::lock_guard<std::mutex> lock(getDrawCommandMutex());
            for (uint32_t slot = 0; slot < command.computeConstantBuffers.size(); ++slot)
            {
                command.computeConstantBuffers[slot] = captureBufferSnapshot(command.computeConstantBuffers[slot], true);
                command.computeConstantBufferVersions[slot] = captureVersionedBufferSlot(command.computeConstantBuffers[slot]);
            }

            if (sourceContext->isDeferredContext)
            {
                deferredContextDrawCommands[sourceContext].push_back(command);
            }
            else
            {
                pendingDrawCommands.push_back(command);
            }
        }

        void Device::enqueueCopyResourceCommand(Context *sourceContext, Render::Object *destination, Render::Object *source)
        {
            if (!destination || !source)
            {
                return;
            }

            Device::DrawCommand command;
            command.commandType = Device::DrawCommand::Type::CopyResource;
            command.copyDestination = destination;
            command.copySource = source;

            std::lock_guard<std::mutex> lock(getDrawCommandMutex());
            if (sourceContext && sourceContext->isDeferredContext)
            {
                deferredContextDrawCommands[sourceContext].push_back(command);
            }
            else
            {
                pendingDrawCommands.push_back(command);
            }
        }

        void Device::enqueueClearDepthStencilCommand(Context *sourceContext, Render::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
        {
            if (!depthBuffer || flags == 0)
            {
                return;
            }

            auto *depthTexture = getObject<DepthTexture>(depthBuffer);
            if (!depthTexture || depthTexture->image == VK_NULL_HANDLE)
            {
                return;
            }

            Device::DrawCommand command;
            command.commandType = Device::DrawCommand::Type::ClearDepthStencil;
            command.clearDepthTarget = depthTexture;
            command.clearDepthStencilFlags = flags;
            command.clearDepthValue = clearDepth;
            command.clearStencilValue = clearStencil;

            std::lock_guard<std::mutex> lock(getDrawCommandMutex());
            if (sourceContext && sourceContext->isDeferredContext)
            {
                deferredContextDrawCommands[sourceContext].push_back(command);
            }
            else
            {
                pendingDrawCommands.push_back(command);
            }
        }

        void Device::enqueueClearRenderTargetCommand(Context *sourceContext, Render::Target *renderTarget, Math::Float4 const &clearColor)
        {
            if (!renderTarget)
            {
                return;
            }

            auto *targetTexture = getObject<TargetTexture>(renderTarget);
            if (!targetTexture || targetTexture->image == VK_NULL_HANDLE)
            {
                return;
            }

            Device::DrawCommand command;
            command.commandType = Device::DrawCommand::Type::ClearRenderTarget;
            command.clearRenderTarget = targetTexture;
            command.clearRenderTargetColor.float32[0] = clearColor.r;
            command.clearRenderTargetColor.float32[1] = clearColor.g;
            command.clearRenderTargetColor.float32[2] = clearColor.b;
            command.clearRenderTargetColor.float32[3] = clearColor.a;

            std::lock_guard<std::mutex> lock(getDrawCommandMutex());
            if (sourceContext && sourceContext->isDeferredContext)
            {
                deferredContextDrawCommands[sourceContext].push_back(command);
            }
            else
            {
                pendingDrawCommands.push_back(command);
            }
        }

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Render::Implementation
}; // namespace Gek
