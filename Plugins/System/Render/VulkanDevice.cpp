#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/RenderDevice.hpp"
#include "GEK/System/WindowDevice.hpp"
#include <algorithm>
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

#ifdef WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <DirectXTex.h>
#ifdef WIN32
#include <objbase.h>
#endif
#include <vulkan/vulkan.h>

#include <slang.h>
#include <slang-com-ptr.h>

namespace Gek
{
    namespace Render::Implementation
    {
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
            default:
                return VK_FORMAT_UNDEFINED;
            }
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
            Render::Buffer::Description description;
            VkDevice device = VK_NULL_HANDLE;
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void *mappedData = nullptr;
            VkDeviceSize size = 0;

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
            }
        };

        template <int UNIQUE>
        class Program
            : public Render::Program
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

                        context->currentVertexConstantBuffer = context->currentVertexConstantBuffers[0];
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

                        context->currentVertexConstantBuffer = context->currentVertexConstantBuffers[0];
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
                        if (!list.empty() && firstStage == 0)
                        {
                            auto samplerState = getObject<SamplerState>(list[0]);
                            context->currentPixelSamplerState = samplerState;
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

                        context->currentPixelConstantBuffer = context->currentPixelConstantBuffers[0];
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

                        if (firstStage == 0 && !context->currentPixelResourceImageViews.empty())
                        {
                            context->currentPixelImageView = context->currentPixelResourceImageViews[0];
                            context->currentPixelImageSampler = context->currentPixelResourceSamplers[0];
                        }
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
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

                        context->currentPixelConstantBuffer = context->currentPixelConstantBuffers[0];
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

                        context->currentPixelImageView = context->currentPixelResourceImageViews[0];
                        context->currentPixelImageSampler = context->currentPixelResourceSamplers[0];
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                    }
                };

            public:
	            Device *owner = nullptr;
    	        VkDevice device;
                bool isDeferredContext = false;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;

                InputLayout *currentInputLayout = nullptr;
                Buffer *currentVertexBuffer = nullptr;
                uint32_t currentVertexBufferOffset = 0;
                std::array<Buffer *, 8> currentVertexBufferList{};
                std::array<uint32_t, 8> currentVertexBufferOffsetList{};
                Buffer *currentIndexBuffer = nullptr;
                uint32_t currentIndexBufferOffset = 0;
                Render::PrimitiveType currentPrimitiveType = Render::PrimitiveType::TriangleList;
                VkRect2D currentScissor = { {0, 0}, {0, 0} };
                TargetTexture *currentRenderTarget = nullptr;
                std::array<TargetTexture *, 8> currentRenderTargetList{};
                uint32_t currentRenderTargetCount = 0;

                VertexProgram *currentVertexProgram = nullptr;
                PixelProgram *currentPixelProgram = nullptr;
                Buffer *currentVertexConstantBuffer = nullptr;
                Buffer *currentPixelConstantBuffer = nullptr;
                std::array<Buffer *, ContextResourceSlotCount> currentVertexConstantBuffers{};
                std::array<Buffer *, ContextResourceSlotCount> currentPixelConstantBuffers{};
                VkImageView currentPixelImageView = VK_NULL_HANDLE;
                VkSampler currentPixelImageSampler = VK_NULL_HANDLE;
                std::array<VkImageView, 16> currentPixelResourceImageViews{};
                std::array<VkSampler, 16> currentPixelResourceSamplers{};
                std::array<Buffer *, ContextResourceSlotCount> currentPixelResourceBuffers{};
                SamplerState *currentPixelSamplerState = nullptr;
                BlendState *currentBlendState = nullptr;
                DepthState *currentDepthState = nullptr;

            public:
                Context(Device *owner, bool isDeferredContext = false)
                    : owner(owner)
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
                }

                void resolveSamples(Render::Texture *destination, Render::Texture *source)
                {
                }

                void clearState(void)
                {
                    currentInputLayout = nullptr;
                    currentVertexBuffer = nullptr;
                    currentVertexBufferList.fill(nullptr);
                    currentVertexBufferOffsetList.fill(0);
                    currentIndexBuffer = nullptr;
                    currentScissor = { {0, 0}, {0, 0} };
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                    currentVertexProgram = nullptr;
                    currentPixelProgram = nullptr;
                    currentVertexConstantBuffer = nullptr;
                    currentPixelConstantBuffer = nullptr;
                    currentVertexConstantBuffers.fill(nullptr);
                    currentPixelConstantBuffers.fill(nullptr);
                    currentPixelImageView = VK_NULL_HANDLE;
                    currentPixelImageSampler = VK_NULL_HANDLE;
                    currentPixelResourceImageViews.fill(VK_NULL_HANDLE);
                    currentPixelResourceSamplers.fill(VK_NULL_HANDLE);
                    currentPixelResourceBuffers.fill(nullptr);
                    currentPixelSamplerState = nullptr;
                    currentBlendState = nullptr;
                    currentDepthState = nullptr;
                }

                void setViewportList(const std::vector<Render::ViewPort> &viewPortList)
                {
                    if (owner && !viewPortList.empty())
                    {
                        const auto &viewPort = viewPortList[0];
                        owner->currentViewport.x = viewPort.position.x;
                        owner->currentViewport.y = viewPort.position.y;
                        owner->currentViewport.width = viewPort.size.x;
                        owner->currentViewport.height = viewPort.size.y;
                        owner->currentViewport.minDepth = viewPort.nearClip;
                        owner->currentViewport.maxDepth = viewPort.farClip;
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
                    if (!owner || !renderTarget)
                    {
                        return;
                    }

                    if (renderTarget == owner->backBuffer.get())
                    {
                        owner->pendingClearColor =
                        {
                            clearColor.r,
                            clearColor.g,
                            clearColor.b,
                            clearColor.a,
                        };
                    }
                }

                void clearDepthStencilTarget(Render::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
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

                    currentVertexBuffer = currentVertexBufferList[0];
                    currentVertexBufferOffset = currentVertexBufferOffsetList[0];
                }

                void clearRenderTargetList(uint32_t count, bool depthBuffer)
                {
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                }

                void setRenderTargetList(const std::vector<Render::Target *> &renderTargetList, Render::Object *depthBuffer)
                {
                    currentRenderTarget = nullptr;
                    currentRenderTargetList.fill(nullptr);
                    currentRenderTargetCount = 0;
                    if (!owner)
                    {
                        return;
                    }

                    for (auto *renderTarget : renderTargetList)
                    {
                        if (!renderTarget || renderTarget == owner->backBuffer.get())
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

                    currentVertexBuffer = currentVertexBufferList[0];
                    currentVertexBufferOffset = currentVertexBufferOffsetList[0];
                }

                void setPrimitiveType(Render::PrimitiveType primitiveType)
                {
                    currentPrimitiveType = primitiveType;
                }

                void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
                {
                    if (!owner)
                    {
                        return;
                    }

                    ++owner->contextDrawCallAttempts;

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++owner->contextDrawSkipsMissingProgram;
                        if (!owner->loggedMissingProgramSkip)
                        {
                            owner->loggedMissingProgramSkip = true;
                            owner->getContext()->log(
                                Gek::Context::Error,
                                "Vulkan draw skip: missing program (vertexProgram={} pixelProgram={})",
                                static_cast<uint32_t>(currentVertexProgram != nullptr),
                                static_cast<uint32_t>(currentPixelProgram != nullptr));
                        }
                        return;
                    }

                    if (vertexCount == 0)
                    {
                        ++owner->contextDrawSkipsZeroCount;
                        if (!owner->loggedZeroCountSkip)
                        {
                            owner->loggedZeroCountSkip = true;
                            owner->getContext()->log(Gek::Context::Warning, "Vulkan draw skip: zero vertex count");
                        }
                        return;
                    }

                    DrawCommand command;
                    command.indexed = false;
                    command.instanceCount = 1;
                    command.firstInstance = 0;
                    command.vertexBuffer = currentVertexBuffer;
                    command.vertexOffset = currentVertexBufferOffset;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.vertexCount = vertexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = owner->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffer = currentVertexConstantBuffer;
                    command.pixelConstantBuffer = currentPixelConstantBuffer;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = command.pixelResourceImageViews[0];
                    command.pixelSampler = command.pixelResourceSamplers[0];
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = (currentPixelSamplerState ? currentPixelSamplerState->sampler : VK_NULL_HANDLE);
                    }
                    command.renderTarget = currentRenderTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = currentPixelImageSampler;
                    }
                    if (command.pixelSampler != VK_NULL_HANDLE && command.pixelResourceSamplers[0] == VK_NULL_HANDLE)
                    {
                        command.pixelResourceSamplers[0] = command.pixelSampler;
                    }
                    if (command.pixelImageView == VK_NULL_HANDLE)
                    {
                        command.pixelImageView = currentPixelImageView;
                        if (command.pixelResourceImageViews[0] == VK_NULL_HANDLE)
                        {
                            command.pixelResourceImageViews[0] = command.pixelImageView;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
                        std::map<Buffer *, Buffer *> constantBufferSnapshotMap;
                        auto snapshotConstantBuffer = [&](Buffer *buffer) -> Buffer *
                        {
                            if (!buffer)
                            {
                                return nullptr;
                            }

                            auto snapshotSearch = constantBufferSnapshotMap.find(buffer);
                            if (snapshotSearch != std::end(constantBufferSnapshotMap))
                            {
                                return snapshotSearch->second;
                            }

                            Buffer *snapshotBuffer = owner->createConstantBufferSnapshot(buffer);
                            constantBufferSnapshotMap[buffer] = snapshotBuffer;
                            return snapshotBuffer;
                        };

                        command.vertexConstantBuffer = snapshotConstantBuffer(command.vertexConstantBuffer);
                        command.pixelConstantBuffer = snapshotConstantBuffer(command.pixelConstantBuffer);
                        for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                        {
                            command.vertexConstantBuffers[slot] = snapshotConstantBuffer(command.vertexConstantBuffers[slot]);
                            command.pixelConstantBuffers[slot] = snapshotConstantBuffer(command.pixelConstantBuffers[slot]);
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
                            owner->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                        }

                        if (isDeferredContext)
                        {
                            owner->deferredContextDrawCommands[this].push_back(command);
                            ++owner->deferredCommandEnqueueCount;
                        }
                        else
                        {
                            owner->pendingDrawCommands.push_back(command);
                            ++owner->pendingCommandEnqueueCount;
                        }
                    }
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    if (!owner)
                    {
                        return;
                    }

                    ++owner->contextDrawCallAttempts;

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++owner->contextDrawSkipsMissingProgram;
                        if (!owner->loggedMissingProgramSkip)
                        {
                            owner->loggedMissingProgramSkip = true;
                            owner->getContext()->log(
                                Gek::Context::Error,
                                "Vulkan draw skip: missing program (vertexProgram={} pixelProgram={})",
                                static_cast<uint32_t>(currentVertexProgram != nullptr),
                                static_cast<uint32_t>(currentPixelProgram != nullptr));
                        }
                        return;
                    }

                    if (vertexCount == 0 || instanceCount == 0)
                    {
                        ++owner->contextDrawSkipsZeroCount;
                        if (!owner->loggedZeroCountSkip)
                        {
                            owner->loggedZeroCountSkip = true;
                            owner->getContext()->log(Gek::Context::Warning, "Vulkan draw skip: zero count in instanced draw");
                        }
                        return;
                    }

                    DrawCommand command;
                    command.indexed = false;
                    command.instanceCount = instanceCount;
                    command.firstInstance = firstInstance;
                    command.vertexBuffer = currentVertexBuffer;
                    command.vertexOffset = currentVertexBufferOffset;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.vertexCount = vertexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = owner->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffer = currentVertexConstantBuffer;
                    command.pixelConstantBuffer = currentPixelConstantBuffer;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = command.pixelResourceImageViews[0];
                    command.pixelSampler = command.pixelResourceSamplers[0];
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = (currentPixelSamplerState ? currentPixelSamplerState->sampler : VK_NULL_HANDLE);
                    }
                    command.renderTarget = currentRenderTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = currentPixelImageSampler;
                    }
                    if (command.pixelSampler != VK_NULL_HANDLE && command.pixelResourceSamplers[0] == VK_NULL_HANDLE)
                    {
                        command.pixelResourceSamplers[0] = command.pixelSampler;
                    }
                    if (command.pixelImageView == VK_NULL_HANDLE)
                    {
                        command.pixelImageView = currentPixelImageView;
                        if (command.pixelResourceImageViews[0] == VK_NULL_HANDLE)
                        {
                            command.pixelResourceImageViews[0] = command.pixelImageView;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
                        std::map<Buffer *, Buffer *> constantBufferSnapshotMap;
                        auto snapshotConstantBuffer = [&](Buffer *buffer) -> Buffer *
                        {
                            if (!buffer)
                            {
                                return nullptr;
                            }

                            auto snapshotSearch = constantBufferSnapshotMap.find(buffer);
                            if (snapshotSearch != std::end(constantBufferSnapshotMap))
                            {
                                return snapshotSearch->second;
                            }

                            Buffer *snapshotBuffer = owner->createConstantBufferSnapshot(buffer);
                            constantBufferSnapshotMap[buffer] = snapshotBuffer;
                            return snapshotBuffer;
                        };

                        command.vertexConstantBuffer = snapshotConstantBuffer(command.vertexConstantBuffer);
                        command.pixelConstantBuffer = snapshotConstantBuffer(command.pixelConstantBuffer);
                        for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                        {
                            command.vertexConstantBuffers[slot] = snapshotConstantBuffer(command.vertexConstantBuffers[slot]);
                            command.pixelConstantBuffers[slot] = snapshotConstantBuffer(command.pixelConstantBuffers[slot]);
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
                            owner->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                        }

                        if (isDeferredContext)
                        {
                            owner->deferredContextDrawCommands[this].push_back(command);
                            ++owner->deferredCommandEnqueueCount;
                        }
                        else
                        {
                            owner->pendingDrawCommands.push_back(command);
                            ++owner->pendingCommandEnqueueCount;
                        }
                    }
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!owner)
                    {
                        return;
                    }

                    ++owner->contextDrawCallAttempts;

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++owner->contextDrawSkipsMissingProgram;
                        if (!owner->loggedMissingProgramSkip)
                        {
                            owner->loggedMissingProgramSkip = true;
                            owner->getContext()->log(
                                Gek::Context::Error,
                                "Vulkan draw skip: missing program (vertexProgram={} pixelProgram={})",
                                static_cast<uint32_t>(currentVertexProgram != nullptr),
                                static_cast<uint32_t>(currentPixelProgram != nullptr));
                        }
                        return;
                    }

                    if (!currentVertexBuffer)
                    {
                        ++owner->contextDrawSkipsMissingVertexBuffer;
                        if (!owner->loggedMissingVertexBufferSkip)
                        {
                            owner->loggedMissingVertexBufferSkip = true;
                            owner->getContext()->log(Gek::Context::Error, "Vulkan draw skip: missing vertex buffer for indexed draw");
                        }
                        return;
                    }

                    if (!currentIndexBuffer)
                    {
                        ++owner->contextDrawSkipsMissingIndexBuffer;
                        if (!owner->loggedMissingIndexBufferSkip)
                        {
                            owner->loggedMissingIndexBufferSkip = true;
                            owner->getContext()->log(Gek::Context::Error, "Vulkan draw skip: missing index buffer for indexed draw");
                        }
                        return;
                    }

                    if (indexCount == 0)
                    {
                        ++owner->contextDrawSkipsZeroCount;
                        if (!owner->loggedZeroCountSkip)
                        {
                            owner->loggedZeroCountSkip = true;
                            owner->getContext()->log(Gek::Context::Warning, "Vulkan draw skip: zero index count");
                        }
                        return;
                    }

                    DrawCommand command;
                    command.indexed = true;
                    command.instanceCount = 1;
                    command.firstInstance = 0;
                    command.vertexBuffer = currentVertexBuffer;
                    command.vertexOffset = currentVertexBufferOffset;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.indexBuffer = currentIndexBuffer;
                    command.indexOffset = currentIndexBufferOffset + firstIndex * (currentIndexBuffer->getDescription().format == Render::Format::R16_UINT ? 2u : 4u);
                    command.indexCount = indexCount;
                    command.firstVertex = firstVertex;
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = owner->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffer = currentVertexConstantBuffer;
                    command.pixelConstantBuffer = currentPixelConstantBuffer;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = command.pixelResourceImageViews[0];
                    command.pixelSampler = command.pixelResourceSamplers[0];
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = (currentPixelSamplerState ? currentPixelSamplerState->sampler : VK_NULL_HANDLE);
                    }
                    command.renderTarget = currentRenderTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = currentPixelImageSampler;
                    }
                    if (command.pixelSampler != VK_NULL_HANDLE && command.pixelResourceSamplers[0] == VK_NULL_HANDLE)
                    {
                        command.pixelResourceSamplers[0] = command.pixelSampler;
                    }
                    if (command.pixelImageView == VK_NULL_HANDLE)
                    {
                        command.pixelImageView = currentPixelImageView;
                        if (command.pixelResourceImageViews[0] == VK_NULL_HANDLE)
                        {
                            command.pixelResourceImageViews[0] = command.pixelImageView;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
                        std::map<Buffer *, Buffer *> constantBufferSnapshotMap;
                        auto snapshotConstantBuffer = [&](Buffer *buffer) -> Buffer *
                        {
                            if (!buffer)
                            {
                                return nullptr;
                            }

                            auto snapshotSearch = constantBufferSnapshotMap.find(buffer);
                            if (snapshotSearch != std::end(constantBufferSnapshotMap))
                            {
                                return snapshotSearch->second;
                            }

                            Buffer *snapshotBuffer = owner->createConstantBufferSnapshot(buffer);
                            constantBufferSnapshotMap[buffer] = snapshotBuffer;
                            return snapshotBuffer;
                        };

                        command.vertexConstantBuffer = snapshotConstantBuffer(command.vertexConstantBuffer);
                        command.pixelConstantBuffer = snapshotConstantBuffer(command.pixelConstantBuffer);
                        for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                        {
                            command.vertexConstantBuffers[slot] = snapshotConstantBuffer(command.vertexConstantBuffers[slot]);
                            command.pixelConstantBuffers[slot] = snapshotConstantBuffer(command.pixelConstantBuffers[slot]);
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
                            owner->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                        }

                        if (isDeferredContext)
                        {
                            owner->deferredContextDrawCommands[this].push_back(command);
                            ++owner->deferredCommandEnqueueCount;
                        }
                        else
                        {
                            owner->pendingDrawCommands.push_back(command);
                            ++owner->pendingCommandEnqueueCount;
                        }
                    }
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    if (!owner)
                    {
                        return;
                    }

                    ++owner->contextDrawCallAttempts;

                    if (!currentVertexProgram || !currentPixelProgram)
                    {
                        ++owner->contextDrawSkipsMissingProgram;
                        if (!owner->loggedMissingProgramSkip)
                        {
                            owner->loggedMissingProgramSkip = true;
                            owner->getContext()->log(
                                Gek::Context::Error,
                                "Vulkan draw skip: missing program (vertexProgram={} pixelProgram={})",
                                static_cast<uint32_t>(currentVertexProgram != nullptr),
                                static_cast<uint32_t>(currentPixelProgram != nullptr));
                        }
                        return;
                    }

                    if (!currentVertexBuffer)
                    {
                        ++owner->contextDrawSkipsMissingVertexBuffer;
                        if (!owner->loggedMissingVertexBufferSkip)
                        {
                            owner->loggedMissingVertexBufferSkip = true;
                            owner->getContext()->log(Gek::Context::Error, "Vulkan draw skip: missing vertex buffer for instanced indexed draw");
                        }
                        return;
                    }

                    if (!currentIndexBuffer)
                    {
                        ++owner->contextDrawSkipsMissingIndexBuffer;
                        if (!owner->loggedMissingIndexBufferSkip)
                        {
                            owner->loggedMissingIndexBufferSkip = true;
                            owner->getContext()->log(Gek::Context::Error, "Vulkan draw skip: missing index buffer for instanced indexed draw");
                        }
                        return;
                    }

                    if (indexCount == 0 || instanceCount == 0)
                    {
                        ++owner->contextDrawSkipsZeroCount;
                        if (!owner->loggedZeroCountSkip)
                        {
                            owner->loggedZeroCountSkip = true;
                            owner->getContext()->log(Gek::Context::Warning, "Vulkan draw skip: zero count in instanced indexed draw");
                        }
                        return;
                    }

                    DrawCommand command;
                    command.indexed = true;
                    command.instanceCount = instanceCount;
                    command.firstInstance = firstInstance;
                    command.vertexBuffer = currentVertexBuffer;
                    command.vertexOffset = currentVertexBufferOffset;
                    command.vertexBuffers = currentVertexBufferList;
                    command.vertexOffsets = currentVertexBufferOffsetList;
                    command.indexBuffer = currentIndexBuffer;
                    command.indexOffset = currentIndexBufferOffset + firstIndex * (currentIndexBuffer->getDescription().format == Render::Format::R16_UINT ? 2u : 4u);
                    command.indexCount = indexCount;
                    command.firstVertex = static_cast<int32_t>(firstVertex);
                    command.primitiveType = currentPrimitiveType;
                    command.scissor = currentScissor;
                    command.viewport = owner->currentViewport;
                    command.inputLayout = currentInputLayout;
                    command.vertexProgram = currentVertexProgram;
                    command.pixelProgram = currentPixelProgram;
                    command.vertexConstantBuffer = currentVertexConstantBuffer;
                    command.pixelConstantBuffer = currentPixelConstantBuffer;
                    command.vertexConstantBuffers = currentVertexConstantBuffers;
                    command.pixelConstantBuffers = currentPixelConstantBuffers;
                    command.pixelResourceImageViews = currentPixelResourceImageViews;
                    command.pixelResourceSamplers = currentPixelResourceSamplers;
                    command.pixelResourceBuffers = currentPixelResourceBuffers;
                    command.pixelImageView = command.pixelResourceImageViews[0];
                    command.pixelSampler = command.pixelResourceSamplers[0];
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = (currentPixelSamplerState ? currentPixelSamplerState->sampler : VK_NULL_HANDLE);
                    }
                    command.renderTarget = currentRenderTarget;
                    command.hasOffscreenTarget = (currentRenderTargetCount > 0);
                    command.offscreenTargetCount = currentRenderTargetCount;
                    command.blendState = currentBlendState;
                    command.depthState = currentDepthState;
                    if (command.pixelSampler == VK_NULL_HANDLE)
                    {
                        command.pixelSampler = currentPixelImageSampler;
                    }
                    if (command.pixelSampler != VK_NULL_HANDLE && command.pixelResourceSamplers[0] == VK_NULL_HANDLE)
                    {
                        command.pixelResourceSamplers[0] = command.pixelSampler;
                    }
                    if (command.pixelImageView == VK_NULL_HANDLE)
                    {
                        command.pixelImageView = currentPixelImageView;
                        if (command.pixelResourceImageViews[0] == VK_NULL_HANDLE)
                        {
                            command.pixelResourceImageViews[0] = command.pixelImageView;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
                        std::map<Buffer *, Buffer *> constantBufferSnapshotMap;
                        auto snapshotConstantBuffer = [&](Buffer *buffer) -> Buffer *
                        {
                            if (!buffer)
                            {
                                return nullptr;
                            }

                            auto snapshotSearch = constantBufferSnapshotMap.find(buffer);
                            if (snapshotSearch != std::end(constantBufferSnapshotMap))
                            {
                                return snapshotSearch->second;
                            }

                            Buffer *snapshotBuffer = owner->createConstantBufferSnapshot(buffer);
                            constantBufferSnapshotMap[buffer] = snapshotBuffer;
                            return snapshotBuffer;
                        };

                        command.vertexConstantBuffer = snapshotConstantBuffer(command.vertexConstantBuffer);
                        command.pixelConstantBuffer = snapshotConstantBuffer(command.pixelConstantBuffer);
                        for (uint32_t slot = 0; slot < command.vertexConstantBuffers.size(); ++slot)
                        {
                            command.vertexConstantBuffers[slot] = snapshotConstantBuffer(command.vertexConstantBuffers[slot]);
                            command.pixelConstantBuffers[slot] = snapshotConstantBuffer(command.pixelConstantBuffers[slot]);
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
                            owner->offscreenImageLayouts.try_emplace(command.offscreenImages[targetIndex], target->currentLayout);
                        }

                        if (isDeferredContext)
                        {
                            owner->deferredContextDrawCommands[this].push_back(command);
                            ++owner->deferredCommandEnqueueCount;
                        }
                        else
                        {
                            owner->pendingDrawCommands.push_back(command);
                            ++owner->pendingCommandEnqueueCount;
                        }
                    }
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                }

                Render::ObjectPtr finishCommandList(void)
                {
                    if (!owner)
                    {
                        return nullptr;
                    }

                    auto commandList = std::make_unique<CommandList>();
                    commandList->identifier = owner->nextCommandListIdentifier++;
                    if (isDeferredContext)
                    {
                        ++owner->deferredFinishCalls;
                        std::lock_guard<std::mutex> lock(Device::getDrawCommandMutex());
                        auto deferredContextCommandIterator = owner->deferredContextDrawCommands.find(this);
                        if (deferredContextCommandIterator != owner->deferredContextDrawCommands.end())
                        {
                            owner->deferredCommandLists[commandList->identifier] = std::move(deferredContextCommandIterator->second);
                            owner->deferredContextDrawCommands.erase(deferredContextCommandIterator);
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
            VkCommandPool commandPool = VK_NULL_HANDLE;
            VkCommandPool uploadCommandPool = VK_NULL_HANDLE;
            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
            std::vector<VkSemaphore> renderFinishedSemaphores;
            VkFence inFlightFence = VK_NULL_HANDLE;
            bool inFlightFencePending = false;
            VkRenderPass renderPass = VK_NULL_HANDLE;
            std::vector<VkFramebuffer> swapChainFramebuffers;
            std::map<std::vector<VkFormat>, VkRenderPass> offscreenRenderPassCache;
            std::vector<VkFramebuffer> transientFramebuffers;

            VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
            VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
            VkPipelineLayout graphicsPipelineLayout = VK_NULL_HANDLE;
            VkDescriptorSetLayout uiDescriptorSetLayout = VK_NULL_HANDLE;
            VkDescriptorPool uiDescriptorPool = VK_NULL_HANDLE;
            VkPipelineLayout uiGraphicsPipelineLayout = VK_NULL_HANDLE;
            static constexpr uint32_t PixelResourceSlotCount = 16;
            static constexpr uint32_t DescriptorSampledImageBase = 0;
            static constexpr uint32_t DescriptorStorageBufferBase = 64;
            static constexpr uint32_t DescriptorVertexUniformBufferBase = 128;
            static constexpr uint32_t DescriptorPixelUniformBufferBase = 144;
            static constexpr uint32_t DescriptorSamplerBase = 192;
            static constexpr uint32_t DescriptorStorageImageBase = 224;

            struct DrawCommand
            {
                bool indexed = false;
                uint32_t instanceCount = 1;
                uint32_t firstInstance = 0;
                Buffer *vertexBuffer = nullptr;
                uint32_t vertexOffset = 0;
                std::array<Buffer *, 8> vertexBuffers{};
                std::array<uint32_t, 8> vertexOffsets{};
                uint32_t vertexCount = 0;
                Buffer *indexBuffer = nullptr;
                uint32_t indexOffset = 0;
                uint32_t indexCount = 0;
                int32_t firstVertex = 0;
                Render::PrimitiveType primitiveType = Render::PrimitiveType::TriangleList;
                VkRect2D scissor = {{ 0, 0 }, { 1, 1 }};
                VkViewport viewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
                InputLayout *inputLayout = nullptr;
                VertexProgram *vertexProgram = nullptr;
                PixelProgram *pixelProgram = nullptr;
                Buffer *vertexConstantBuffer = nullptr;
                Buffer *pixelConstantBuffer = nullptr;
                std::array<Buffer *, PixelResourceSlotCount> vertexConstantBuffers{};
                std::array<Buffer *, PixelResourceSlotCount> pixelConstantBuffers{};
                VkImageView pixelImageView = VK_NULL_HANDLE;
                VkSampler pixelSampler = VK_NULL_HANDLE;
                std::array<VkImageView, PixelResourceSlotCount> pixelResourceImageViews{};
                std::array<VkSampler, PixelResourceSlotCount> pixelResourceSamplers{};
                std::array<Buffer *, PixelResourceSlotCount> pixelResourceBuffers{};
                TargetTexture *renderTarget = nullptr;
                bool hasOffscreenTarget = false;
                uint32_t offscreenTargetCount = 0;
                std::array<VkImage, 8> offscreenImages{};
                std::array<VkImageView, 8> offscreenImageViews{};
                std::array<VkFormat, 8> offscreenFormats{};
                std::array<VkExtent2D, 8> offscreenExtents{};
                BlendState *blendState = nullptr;
                DepthState *depthState = nullptr;
            };

            std::vector<DrawCommand> pendingDrawCommands;
            std::map<Context *, std::vector<DrawCommand>> deferredContextDrawCommands;
            std::map<uint64_t, std::vector<DrawCommand>> deferredCommandLists;
            std::vector<Render::BufferPtr> transientFrameConstantBufferSnapshotsPending;
            std::vector<Render::BufferPtr> transientFrameConstantBufferSnapshotsInFlight;
            uint64_t nextCommandListIdentifier = 1;
            uint64_t deferredFinishCalls = 0;
            uint64_t deferredExecuteCalls = 0;
            uint64_t deferredExecutedCommandCount = 0;
            uint64_t pendingCommandEnqueueCount = 0;
            uint64_t deferredCommandEnqueueCount = 0;
            std::map<VkImage, VkImageLayout> offscreenImageLayouts;
            uint64_t presentFrameIndex = 0;
            bool loggedMrtFallback = false;
            VkViewport currentViewport = { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
            uint64_t contextDrawCallAttempts = 0;
            uint64_t contextDrawSkipsMissingProgram = 0;
            uint64_t contextDrawSkipsMissingVertexBuffer = 0;
            uint64_t contextDrawSkipsMissingIndexBuffer = 0;
            uint64_t contextDrawSkipsZeroCount = 0;
            bool loggedMissingProgramSkip = false;
            bool loggedMissingVertexBufferSkip = false;
            bool loggedMissingIndexBufferSkip = false;
            bool loggedZeroCountSkip = false;
            bool deviceLost = false;
            bool loggedDeviceLost = false;
            std::string debugOverlayText = "VK: init";

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

                bool operator < (const PipelineKey &other) const
                {
                    if (vertexModule != other.vertexModule) return vertexModule < other.vertexModule;
                    if (pixelModule != other.pixelModule) return pixelModule < other.pixelModule;
                    if (inputLayout != other.inputLayout) return inputLayout < other.inputLayout;
                    if (renderPass != other.renderPass) return renderPass < other.renderPass;
                    if (primitiveType != other.primitiveType) return primitiveType < other.primitiveType;
                    if (blendEnabled != other.blendEnabled) return blendEnabled < other.blendEnabled;
                    if (depthEnabled != other.depthEnabled) return depthEnabled < other.depthEnabled;
                    return depthWrite < other.depthWrite;
                }
            };

            std::map<PipelineKey, VkPipeline> graphicsPipelineCache;
            std::set<PipelineKey> failedGraphicsPipelineKeys;

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

            Buffer *createConstantBufferSnapshot(Buffer *sourceBuffer)
            {
                if (!sourceBuffer || sourceBuffer->buffer == VK_NULL_HANDLE)
                {
                    return nullptr;
                }

                if (sourceBuffer->getDescription().type != Render::Buffer::Type::Constant)
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

                transientFrameConstantBufferSnapshotsPending.push_back(std::move(snapshotResource));
                return snapshotBuffer;
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

                if (!availableDeviceFeatures.shaderStorageImageReadWithoutFormat || !availableDeviceFeatures.shaderStorageImageWriteWithoutFormat)
                {
                    throw std::runtime_error("Vulkan device missing required formatless storage image read/write features");
                }

                deviceFeatures.shaderStorageImageReadWithoutFormat = VK_TRUE;
                deviceFeatures.shaderStorageImageWriteWithoutFormat = VK_TRUE;

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

                for (auto framebuffer : swapChainFramebuffers)
                {
                    vkDestroyFramebuffer(device, framebuffer, nullptr);
                }
                swapChainFramebuffers.clear();

                if (renderPass != VK_NULL_HANDLE)
                {
                    vkDestroyRenderPass(device, renderPass, nullptr);
                    renderPass = VK_NULL_HANDLE;
                }
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

                VkAttachmentReference colorAttachmentRef{};
                colorAttachmentRef.attachment = 0;
                colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &colorAttachmentRef;

                VkSubpassDependency dependency{};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = 1;
                renderPassInfo.pAttachments = &colorAttachment;
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
                    VkImageView attachment = swapChainImageViews[imageIndex];
                    VkFramebufferCreateInfo framebufferInfo{};
                    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    framebufferInfo.renderPass = renderPass;
                    framebufferInfo.attachmentCount = 1;
                    framebufferInfo.pAttachments = &attachment;
                    framebufferInfo.width = swapChainExtent.width;
                    framebufferInfo.height = swapChainExtent.height;
                    framebufferInfo.layers = 1;

                    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[imageIndex]) != VK_SUCCESS)
                    {
                        throw std::runtime_error("failed to create Vulkan framebuffer");
                    }
                }
            }

            VkRenderPass getOrCreateOffscreenRenderPass(const std::vector<VkFormat> &formats)
            {
                if (formats.empty())
                {
                    return VK_NULL_HANDLE;
                }

                auto renderPassSearch = offscreenRenderPassCache.find(formats);
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

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
                subpass.pColorAttachments = colorAttachmentRefs.data();

                VkSubpassDependency dependency{};
                dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass = 0;
                dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                renderPassInfo.attachmentCount = static_cast<uint32_t>(colorAttachments.size());
                renderPassInfo.pAttachments = colorAttachments.data();
                renderPassInfo.subpassCount = 1;
                renderPassInfo.pSubpasses = &subpass;
                renderPassInfo.dependencyCount = 1;
                renderPassInfo.pDependencies = &dependency;

                VkRenderPass offscreenRenderPass = VK_NULL_HANDLE;
                if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &offscreenRenderPass) != VK_SUCCESS)
                {
                    return VK_NULL_HANDLE;
                }

                offscreenRenderPassCache[formats] = offscreenRenderPass;
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
                    sampledImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(sampledImageBinding);

                    VkDescriptorSetLayoutBinding storageBufferBinding{};
                    storageBufferBinding.binding = DescriptorStorageBufferBase + slot;
                    storageBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                    storageBufferBinding.descriptorCount = 1;
                    storageBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(storageBufferBinding);

                    VkDescriptorSetLayoutBinding vertexUniformBinding{};
                    vertexUniformBinding.binding = DescriptorVertexUniformBufferBase + slot;
                    vertexUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    vertexUniformBinding.descriptorCount = 1;
                    vertexUniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
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
                    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                    bindings.push_back(samplerBinding);

                    VkDescriptorSetLayoutBinding storageImageBinding{};
                    storageImageBinding.binding = DescriptorStorageImageBase + slot;
                    storageImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                    storageImageBinding.descriptorCount = 1;
                    storageImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
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

                VkDescriptorSetLayoutBinding uiUniformBinding{};
                uiUniformBinding.binding = 0;
                uiUniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                uiUniformBinding.descriptorCount = 1;
                uiUniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                VkDescriptorSetLayoutBinding uiSampledImageBinding{};
                uiSampledImageBinding.binding = 1;
                uiSampledImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                uiSampledImageBinding.descriptorCount = 1;
                uiSampledImageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                VkDescriptorSetLayoutBinding uiSamplerBinding{};
                uiSamplerBinding.binding = 2;
                uiSamplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                uiSamplerBinding.descriptorCount = 1;
                uiSamplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                std::array<VkDescriptorSetLayoutBinding, 3> uiBindings =
                {
                    uiUniformBinding,
                    uiSampledImageBinding,
                    uiSamplerBinding,
                };

                VkDescriptorSetLayoutCreateInfo uiSetLayoutInfo{};
                uiSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                uiSetLayoutInfo.bindingCount = static_cast<uint32_t>(uiBindings.size());
                uiSetLayoutInfo.pBindings = uiBindings.data();
                if (vkCreateDescriptorSetLayout(device, &uiSetLayoutInfo, nullptr, &uiDescriptorSetLayout) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create UI descriptor set layout");
                }

                std::array<VkDescriptorPoolSize, 3> uiPoolSizes =
                {
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
                    VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
                };

                VkDescriptorPoolCreateInfo uiPoolInfo{};
                uiPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                uiPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                uiPoolInfo.maxSets = 1024;
                uiPoolInfo.poolSizeCount = static_cast<uint32_t>(uiPoolSizes.size());
                uiPoolInfo.pPoolSizes = uiPoolSizes.data();
                if (vkCreateDescriptorPool(device, &uiPoolInfo, nullptr, &uiDescriptorPool) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create UI descriptor pool");
                }

                VkPipelineLayoutCreateInfo uiPipelineLayoutInfo{};
                uiPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                uiPipelineLayoutInfo.setLayoutCount = 1;
                uiPipelineLayoutInfo.pSetLayouts = &uiDescriptorSetLayout;
                if (vkCreatePipelineLayout(device, &uiPipelineLayoutInfo, nullptr, &uiGraphicsPipelineLayout) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create UI graphics pipeline layout");
                }
            }

            VkPipeline getOrCreateGraphicsPipeline(const DrawCommand &command, VkRenderPass activeRenderPass)
            {
                if (!command.vertexProgram || !command.pixelProgram)
                {
                    return VK_NULL_HANDLE;
                }

                const bool isUiProgram =
                    (command.pixelProgram &&
                    (
                        command.pixelProgram->getInformation().name.find("core::uiPixelProgram") != std::string::npos ||
                        command.pixelProgram->getInformation().name.find("core:uiPixelProgram") != std::string::npos ||
                        command.pixelProgram->getInformation().name.find("uiPixelProgram") != std::string::npos
                    ));

                PipelineKey key;
                key.vertexModule = command.vertexProgram->shaderModule;
                key.pixelModule = command.pixelProgram->shaderModule;
                key.inputLayout = command.inputLayout;
                key.renderPass = activeRenderPass;
                key.primitiveType = command.primitiveType;
                key.blendEnabled = (command.blendState ? command.blendState->getDescription().targetStates[0].enable : false);
                key.depthEnabled = (command.depthState ? command.depthState->getDescription().enable : false);
                key.depthWrite = (command.depthState ? (command.depthState->getDescription().writeMask != Render::DepthState::Write::Zero) : false);

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
                        if (isUiProgram)
                        {
                            switch (element.semantic)
                            {
                            case Render::InputElement::Semantic::Position:
                                attribute.location = 0;
                                break;
                            case Render::InputElement::Semantic::Color:
                                attribute.location = 1;
                                break;
                            case Render::InputElement::Semantic::TexCoord:
                                attribute.location = 2;
                                break;
                            default:
                                attribute.location = index;
                                break;
                            }
                        }
                        else
                        {
                            attribute.location = index;
                        }
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
                rasterizer.cullMode = VK_CULL_MODE_NONE;
                rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
                rasterizer.depthBiasEnable = VK_FALSE;

                VkPipelineMultisampleStateCreateInfo multisampling{};
                multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
                multisampling.sampleShadingEnable = VK_FALSE;
                multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

                VkPipelineDepthStencilStateCreateInfo depthStencil{};
                depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
                depthStencil.depthTestEnable = key.depthEnabled ? VK_TRUE : VK_FALSE;
                depthStencil.depthWriteEnable = key.depthWrite ? VK_TRUE : VK_FALSE;
                depthStencil.depthCompareOp = key.depthEnabled ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_ALWAYS;
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
                pipelineInfo.layout = (isUiProgram ? uiGraphicsPipelineLayout : graphicsPipelineLayout);
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

                if (uiGraphicsPipelineLayout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(device, uiGraphicsPipelineLayout, nullptr);
                    uiGraphicsPipelineLayout = VK_NULL_HANDLE;
                }

                if (descriptorPool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
                    descriptorPool = VK_NULL_HANDLE;
                }

                if (uiDescriptorPool != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorPool(device, uiDescriptorPool, nullptr);
                    uiDescriptorPool = VK_NULL_HANDLE;
                }

                if (descriptorSetLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
                    descriptorSetLayout = VK_NULL_HANDLE;
                }

                if (uiDescriptorSetLayout != VK_NULL_HANDLE)
                {
                    vkDestroyDescriptorSetLayout(device, uiDescriptorSetLayout, nullptr);
                    uiDescriptorSetLayout = VK_NULL_HANDLE;
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

            std::string getDebugOverlayText(void) const
            {
                return debugOverlayText;
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

                    getContext()->log(Gek::Context::Info, "Display Mode: {} x {}", displayMode.width, displayMode.height);

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

                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.anisotropyEnable = VK_FALSE;
                samplerInfo.maxAnisotropy = 1.0f;
                samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
                samplerInfo.unnormalizedCoordinates = VK_FALSE;
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
                samplerInfo.minLod = 0.0f;
                samplerInfo.maxLod = 0.0f;
                samplerInfo.mipLodBias = 0.0f;
                if (vkCreateSampler(device, &samplerInfo, nullptr, &samplerState->sampler) != VK_SUCCESS)
                {
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
                    vkUnmapMemory(device, vulkanBuffer->memory);
                    vulkanBuffer->mappedData = nullptr;
                }
            }

            void updateResource(Render::Object *object, const void *data)
            {
                auto vulkanBuffer = getObject<Buffer>(object);
                if (vulkanBuffer && data)
                {
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
            }

            std::string_view const getSemanticMoniker(Render::InputElement::Semantic semantic)
            {
                return SemanticNameList[static_cast<uint8_t>(semantic)];
            }

            Render::ObjectPtr createInputLayout(const std::vector<Render::InputElement> &elementList, Render::Program::Information const &information)
            {
                return std::make_unique<InputLayout>(elementList);
            }

            Render::Program::Information compileProgram(Render::Program::Type type, std::string_view name, FileSystem::Path const &debugPath, std::string_view uncompiledProgram, std::string_view entryFunction, std::function<bool(Render::IncludeType, std::string_view, void const **data, uint32_t *size)> &&onInclude)
            {
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
                            addUnique(candidates, includeName + ".hlsl");
                            addUnique(candidates, forwardSlashes + ".slang");
                            addUnique(candidates, forwardSlashes + ".hlsl");
                            addUnique(candidates, backSlashes + ".slang");
                            addUnique(candidates, backSlashes + ".hlsl");
                        }
                        else if ((includeName.substr(extensionPosition) == ".slang") || (includeName.substr(extensionPosition) == ".SLANG"))
                        {
                            std::string hlslName = includeName;
                            hlslName.replace(extensionPosition, std::string::npos, ".hlsl");
                            addUnique(candidates, hlslName);
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

                std::string resolvedProgram = resolveIncludes(uncompiledProgram, 0);

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
                                    (type == Render::Program::Type::Pixel)
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
                                prefix = std::format("[[vk::binding({}, 0)]] ", DescriptorStorageImageBase + registerIndex);
                            }
                            else if (extractRegisterIndex(line, 't', registerIndex))
                            {
                                if (line.find("StructuredBuffer") != std::string_view::npos ||
                                    line.find("ByteAddressBuffer") != std::string_view::npos ||
                                    line.find("RWStructuredBuffer") != std::string_view::npos ||
                                    line.find("Buffer<") != std::string_view::npos)
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

                const bool isUiVertexProgram = (name == "core::uiVertexProgram") || (name == "core:uiVertexProgram");
                const bool isUiPixelProgram = (name == "core::uiPixelProgram") || (name == "core:uiPixelProgram");

                if (isUiVertexProgram)
                {
                    static constexpr std::string_view from = "cbuffer DataBuffer : register(b0)";
                    static constexpr std::string_view to = "[[vk::binding(0, 0)]] cbuffer DataBuffer : register(b0)";
                    size_t position = resolvedProgram.find(from);
                    if (position != std::string::npos)
                    {
                        resolvedProgram.replace(position, from.size(), to);
                    }
                }
                else if (isUiPixelProgram)
                {
                    static constexpr std::string_view textureFrom = "Texture2D<float4> uiTexture : register(t0);";
                    static constexpr std::string_view textureTo = "[[vk::binding(1, 0)]] Texture2D<float4> uiTexture : register(t0);";
                    static constexpr std::string_view samplerFrom = "SamplerState uiSampler : register(s0);";
                    static constexpr std::string_view samplerTo = "[[vk::binding(2, 0)]] SamplerState uiSampler : register(s0);";
                    size_t texturePosition = resolvedProgram.find(textureFrom);
                    if (texturePosition != std::string::npos)
                    {
                        resolvedProgram.replace(texturePosition, textureFrom.size(), textureTo);
                    }
                    else
                    {
                        getContext()->log(Gek::Context::Warning, "Vulkan UI pixel texture binding annotation not applied for '{}'", name);
                    }

                    size_t samplerPosition = resolvedProgram.find(samplerFrom);
                    if (samplerPosition != std::string::npos)
                    {
                        resolvedProgram.replace(samplerPosition, samplerFrom.size(), samplerTo);
                    }
                    else
                    {
                        getContext()->log(Gek::Context::Warning, "Vulkan UI pixel sampler binding annotation not applied for '{}'", name);
                    }
                }
                else
                {
                }

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
                    return {};
                }

                const std::string moduleName(name);
                const std::string debugFileName(debugPath.getFileName());
                const std::string entryPointName(entryFunction);

                slang::IBlob* outDiagnosticsRaw = nullptr;
                slang::IModule* slangModule = session->loadModuleFromSourceString(moduleName.c_str(), debugFileName.c_str(), resolvedProgram.c_str(), &outDiagnosticsRaw);
                Slang::ComPtr<slang::IBlob> outDiagnostics(outDiagnosticsRaw);
                if (!slangModule)
                {
                    const char *diagMsg = outDiagnostics ? reinterpret_cast<const char *>(outDiagnostics->getBufferPointer()) : "Unknown error";
                    getContext()->log(Gek::Context::Error, "Failed to load Slang module from source string: {}", diagMsg);
                    return {};
                }

                Slang::ComPtr<slang::IEntryPoint> entryPoint;
                slangModule->findEntryPointByName(entryPointName.c_str(), entryPoint.writeRef());
                if (!entryPoint)
                {
                    getContext()->log(Gek::Context::Error, "Failed to find entry point '{}' in Slang module", entryFunction);
                    return {};
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
                    return {};
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
                    return {};
                }

                Render::Program::Information information;
                information.type = type;
                information.name = name;
                information.debugPath = debugPath;
                information.entryFunction = entryFunction;
                information.uncompiledData = resolvedProgram;
                information.compiledData = spirvCode ? std::vector<uint8_t>((uint8_t*)spirvCode->getBufferPointer(), (uint8_t*)spirvCode->getBufferPointer() + spirvCode->getBufferSize()) : std::vector<uint8_t>();
                return information;
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

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data)
            {
                if (description.flags & Render::Texture::Flags::RenderTarget)
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
                    imageInfo.mipLevels = 1;
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
                    imageViewInfo.subresourceRange.levelCount = 1;
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
                    samplerInfo.maxLod = 0.0f;
                    if (vkCreateSampler(device, &samplerInfo, nullptr, &texture->sampler) != VK_SUCCESS)
                    {
                        return nullptr;
                    }

                    return texture;
                }
                else if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    return std::make_unique<DepthTexture>(description);
                }
                else
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
            }

            Render::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                std::lock_guard<std::mutex> decodeLock(getTextureDecodeMutex());

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

                if (extension == ".dds")
                {
                    decodeResult = ::DirectX::LoadFromDDSMemory(sourceData.data(), sourceData.size(), ::DirectX::DDS_FLAGS_NONE, nullptr, decodedImage);
                }
                else if (extension == ".tga")
                {
                    decodeResult = ::DirectX::LoadFromTGAMemory(sourceData.data(), sourceData.size(), nullptr, decodedImage);
                }
                else
                {
                    auto wicFlags = (flags & Render::TextureLoadFlags::sRGB) ? ::DirectX::WIC_FLAGS_NONE : ::DirectX::WIC_FLAGS_IGNORE_SRGB;
                    wicFlags |= ::DirectX::WIC_FLAGS_FORCE_RGB;
                    decodeResult = ::DirectX::LoadFromWICMemory(sourceData.data(), sourceData.size(), wicFlags, nullptr, decodedImage);
                }

                if (FAILED(decodeResult))
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: decode error for '{}'", filePath.getString());
                    return nullptr;
                }

                const auto metadata = decodedImage.GetMetadata();
                if (metadata.width == 0 || metadata.height == 0 || metadata.depth == 0)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: invalid metadata for '{}'", filePath.getString());
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
                        getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: RGBA conversion error for '{}'", filePath.getString());
                        return nullptr;
                    }

                    image = convertedImage.GetImage(0, 0, 0);
                }
                if (!image || !image->pixels)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: converted image missing pixels for '{}'", filePath.getString());
                    return nullptr;
                }

                const uint32_t width = static_cast<uint32_t>(image->width);
                const uint32_t height = static_cast<uint32_t>(image->height);
                if (width == 0 || height == 0)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan loadTexture failed: converted image has zero extent for '{}'", filePath.getString());
                    return nullptr;
                }

                constexpr uint32_t kMaxTextureDimension = 2048;
                ::DirectX::ScratchImage resizedImage;
                const ::DirectX::Image *uploadImage = image;
                if (width > kMaxTextureDimension || height > kMaxTextureDimension)
                {
                    const uint32_t maxDimension = std::max(width, height);
                    const uint32_t scaledWidth = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(width) * kMaxTextureDimension) / maxDimension));
                    const uint32_t scaledHeight = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(height) * kMaxTextureDimension) / maxDimension));

                    if (SUCCEEDED(::DirectX::Resize(*image, scaledWidth, scaledHeight, ::DirectX::TEX_FILTER_DEFAULT, resizedImage)))
                    {
                        const ::DirectX::Image *resizedLevel = resizedImage.GetImage(0, 0, 0);
                        if (resizedLevel && resizedLevel->pixels)
                        {
                            uploadImage = resizedLevel;
                            getContext()->log(
                                Gek::Context::Warning,
                                "Vulkan loadTexture downscaled '{}' from {}x{} to {}x{} to reduce GPU memory pressure",
                                filePath.getString(),
                                width,
                                height,
                                scaledWidth,
                                scaledHeight);
                        }
                    }
                }

                const uint32_t uploadWidth = static_cast<uint32_t>(uploadImage->width);
                const uint32_t uploadHeight = static_cast<uint32_t>(uploadImage->height);

                const size_t requiredRowPitch = static_cast<size_t>(uploadWidth) * 4u;
                if (uploadImage->rowPitch < requiredRowPitch)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan loadTexture failed: converted row pitch too small for '{}' (rowPitch={} required={})",
                        filePath.getString(),
                        static_cast<uint64_t>(uploadImage->rowPitch),
                        static_cast<uint64_t>(requiredRowPitch));
                    return nullptr;
                }

                std::vector<uint8_t> uploadData(static_cast<size_t>(uploadWidth) * static_cast<size_t>(uploadHeight) * 4u);
                for (uint32_t row = 0; row < uploadHeight; ++row)
                {
                    const uint8_t *sourceRow = uploadImage->pixels + (static_cast<size_t>(row) * uploadImage->rowPitch);
                    uint8_t *destRow = uploadData.data() + (static_cast<size_t>(row) * requiredRowPitch);
                    std::memcpy(destRow, sourceRow, requiredRowPitch);
                }

                Render::Texture::Description description;
                description.name = filePath.getString();
                description.format = (flags & Render::TextureLoadFlags::sRGB)
                    ? Render::Format::R8G8B8A8_UNORM_SRGB
                    : Render::Format::R8G8B8A8_UNORM;
                description.width = uploadWidth;
                description.height = uploadHeight;
                description.depth = 1;
                description.mipMapCount = 1;
                description.flags = Render::Texture::Flags::Resource;
                return createTexture(description, uploadData.data());
            }

            Render::TexturePtr loadTexture(void const *buffer, size_t size, uint32_t flags)
            {
                std::lock_guard<std::mutex> decodeLock(getTextureDecodeMutex());

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

                constexpr uint32_t kMaxTextureDimension = 2048;
                ::DirectX::ScratchImage resizedImage;
                const ::DirectX::Image *uploadImage = image;
                if (width > kMaxTextureDimension || height > kMaxTextureDimension)
                {
                    const uint32_t maxDimension = std::max(width, height);
                    const uint32_t scaledWidth = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(width) * kMaxTextureDimension) / maxDimension));
                    const uint32_t scaledHeight = std::max(1u, static_cast<uint32_t>((static_cast<uint64_t>(height) * kMaxTextureDimension) / maxDimension));

                    if (SUCCEEDED(::DirectX::Resize(*image, scaledWidth, scaledHeight, ::DirectX::TEX_FILTER_DEFAULT, resizedImage)))
                    {
                        const ::DirectX::Image *resizedLevel = resizedImage.GetImage(0, 0, 0);
                        if (resizedLevel && resizedLevel->pixels)
                        {
                            uploadImage = resizedLevel;
                        }
                    }
                }

                const uint32_t uploadWidth = static_cast<uint32_t>(uploadImage->width);
                const uint32_t uploadHeight = static_cast<uint32_t>(uploadImage->height);

                const size_t requiredRowPitch = static_cast<size_t>(uploadWidth) * 4u;
                if (uploadImage->rowPitch < requiredRowPitch)
                {
                    getContext()->log(
                        Gek::Context::Error,
                        "Vulkan loadTexture(memory) failed: converted row pitch too small (rowPitch={} required={})",
                        static_cast<uint64_t>(uploadImage->rowPitch),
                        static_cast<uint64_t>(requiredRowPitch));
                    return nullptr;
                }

                std::vector<uint8_t> uploadData(static_cast<size_t>(uploadWidth) * static_cast<size_t>(uploadHeight) * 4u);
                for (uint32_t row = 0; row < uploadHeight; ++row)
                {
                    const uint8_t *sourceRow = uploadImage->pixels + (static_cast<size_t>(row) * uploadImage->rowPitch);
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
                return createTexture(description, uploadData.data());
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

                ++deferredExecuteCalls;
                deferredExecutedCommandCount += static_cast<uint64_t>(deferredListIterator->second.size());
                pendingDrawCommands.insert(pendingDrawCommands.end(), deferredListIterator->second.begin(), deferredListIterator->second.end());
                deferredCommandLists.erase(deferredListIterator);
            }

            void present(bool waitForVerticalSync)
            {
                if (device == VK_NULL_HANDLE || swapChain == VK_NULL_HANDLE)
                {
                    return;
                }

                if (deviceLost)
                {
                    std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                    pendingDrawCommands.clear();
                    debugOverlayText = "VK: deviceLost=1";
                    return;
                }

                auto clearQueuedDrawCommands = [&]()
                {
                    std::lock_guard<std::mutex> lock(getDrawCommandMutex());
                    pendingDrawCommands.clear();
                    transientFrameConstantBufferSnapshotsPending.clear();
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
                    debugOverlayText = std::format("VK: deviceLost=1 op={} code={}", operation, static_cast<int32_t>(errorCode));
                    clearQueuedDrawCommands();
                };

                if (inFlightFencePending)
                {
                    const VkResult frameWaitResult = vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
                    if (frameWaitResult != VK_SUCCESS)
                    {
                        handleDeviceLost(frameWaitResult, "vkWaitForFences(inFlightFence)");
                        return;
                    }

                    inFlightFencePending = false;
                }

                for (auto framebuffer : transientFramebuffers)
                {
                    if (framebuffer != VK_NULL_HANDLE)
                    {
                        vkDestroyFramebuffer(device, framebuffer, nullptr);
                    }
                }
                transientFramebuffers.clear();
                transientFrameConstantBufferSnapshotsInFlight.clear();

                if (descriptorPool != VK_NULL_HANDLE)
                {
                    vkResetDescriptorPool(device, descriptorPool, 0);
                }

                if (uiDescriptorPool != VK_NULL_HANDLE)
                {
                    vkResetDescriptorPool(device, uiDescriptorPool, 0);
                }

                uint32_t imageIndex = 0;
                VkResult acquireResult = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
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

                std::lock_guard<std::mutex> drawCommandLock(getDrawCommandMutex());
                const bool hasDrawCommands = !pendingDrawCommands.empty();
                std::vector<VkDescriptorSet> descriptorSets;
                uint32_t sceneCommandCount = 0;
                uint32_t sceneDrawCallsIssued = 0;
                uint32_t sceneBackBufferDrawCalls = 0;
                uint32_t sceneOffscreenDrawCalls = 0;
                uint32_t sceneBackBufferMissingImageDrawCalls = 0;
                bool sceneFallbackCopyPerformed = false;
                uint32_t sceneOffscreenInvalidSkips = 0;
                uint32_t scenePipelineSkips = 0;
                uint32_t sceneDescriptorSkips = 0;
                VkImage sceneOffscreenCopySourceImage = VK_NULL_HANDLE;
                VkExtent2D sceneOffscreenCopySourceExtent = { 0, 0 };
                std::map<VkImageView, std::pair<VkImage, VkExtent2D>> frameOffscreenViewLookup;
                if (hasDrawCommands)
                {
                    transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

                    for (const auto &drawCommand : pendingDrawCommands)
                    {
                        const bool isUiDraw =
                            ((drawCommand.pixelProgram &&
                                (
                                    drawCommand.pixelProgram->getInformation().name.find("core::uiPixelProgram") != std::string::npos ||
                                    drawCommand.pixelProgram->getInformation().name.find("core:uiPixelProgram") != std::string::npos ||
                                    drawCommand.pixelProgram->getInformation().name.find("uiPixelProgram") != std::string::npos
                                )) ||
                            (drawCommand.vertexProgram &&
                                (
                                    drawCommand.vertexProgram->getInformation().name.find("core::uiVertexProgram") != std::string::npos ||
                                    drawCommand.vertexProgram->getInformation().name.find("core:uiVertexProgram") != std::string::npos ||
                                    drawCommand.vertexProgram->getInformation().name.find("uiVertexProgram") != std::string::npos
                                )));
                        if (!isUiDraw)
                        {
                            ++sceneCommandCount;
                        }
                        const bool drawToBackBuffer = !drawCommand.hasOffscreenTarget;

                        VkRenderPass activeRenderPass = renderPass;
                        VkFramebuffer activeFramebuffer = swapChainFramebuffers[imageIndex];
                        VkExtent2D activeExtent = swapChainExtent;
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
                                if (!isUiDraw)
                                {
                                    ++sceneOffscreenInvalidSkips;
                                }
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
                                if (!isUiDraw)
                                {
                                    ++sceneOffscreenInvalidSkips;
                                }
                                continue;
                            }

                            activeRenderPass = getOrCreateOffscreenRenderPass(targetFormats);
                            if (activeRenderPass == VK_NULL_HANDLE)
                            {
                                if (!isUiDraw)
                                {
                                    ++sceneOffscreenInvalidSkips;
                                }
                                continue;
                            }

                            activeExtent = drawCommand.offscreenExtents[0];
                            if (!isUiDraw && offscreenTargetCount > 0)
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

                            VkFramebufferCreateInfo framebufferInfo{};
                            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                            framebufferInfo.renderPass = activeRenderPass;
                            framebufferInfo.attachmentCount = offscreenTargetCount;
                            framebufferInfo.pAttachments = offscreenImageViews.data();
                            framebufferInfo.width = activeExtent.width;
                            framebufferInfo.height = activeExtent.height;
                            framebufferInfo.layers = 1;
                            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &activeFramebuffer) != VK_SUCCESS)
                            {
                                if (!isUiDraw)
                                {
                                    ++sceneOffscreenInvalidSkips;
                                }
                                continue;
                            }
                            transientFramebuffers.push_back(activeFramebuffer);
                        }

                        VkRenderPassBeginInfo renderPassBeginInfo{};
                        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassBeginInfo.renderPass = activeRenderPass;
                        renderPassBeginInfo.framebuffer = activeFramebuffer;
                        renderPassBeginInfo.renderArea.offset = { 0, 0 };
                        renderPassBeginInfo.renderArea.extent = activeExtent;
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
                            }
                        };

                        VkPipeline pipeline = getOrCreateGraphicsPipeline(drawCommand, activeRenderPass);
                        if (pipeline == VK_NULL_HANDLE)
                        {
                            if (!isUiDraw)
                            {
                                ++scenePipelineSkips;
                            }
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
                        if (isUiDraw)
                        {
                            scissor.offset = { 0, 0 };
                            scissor.extent = activeExtent;
                        }
                        else if (scissor.extent.width == 0 || scissor.extent.height == 0)
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

                            VkBuffer vertexBuffer = drawCommand.vertexBuffers[slot]->buffer;
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
                            vkCmdBindIndexBuffer(commandBuffer, drawCommand.indexBuffer->buffer, drawCommand.indexOffset, indexType);
                        }

                        if (!isUiDraw && drawToBackBuffer)
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

                        if (isUiDraw)
                        {
                            if (uiDescriptorSetLayout != VK_NULL_HANDLE && uiDescriptorPool != VK_NULL_HANDLE && uiGraphicsPipelineLayout != VK_NULL_HANDLE)
                            {
                                VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
                                descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                                descriptorAllocateInfo.descriptorPool = uiDescriptorPool;
                                descriptorAllocateInfo.descriptorSetCount = 1;
                                descriptorAllocateInfo.pSetLayouts = &uiDescriptorSetLayout;

                                VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
                                if (vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSet) == VK_SUCCESS)
                                {
                                    VkDescriptorBufferInfo bufferInfo{};
                                    bufferInfo.buffer = drawCommand.vertexConstantBuffer ? drawCommand.vertexConstantBuffer->buffer : VK_NULL_HANDLE;
                                    bufferInfo.offset = 0;
                                    bufferInfo.range = drawCommand.vertexConstantBuffer ? drawCommand.vertexConstantBuffer->size : 0;

                                    VkDescriptorImageInfo sampledImageInfo{};
                                    sampledImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                                    sampledImageInfo.imageView = drawCommand.pixelImageView;

                                    VkDescriptorImageInfo samplerInfo{};
                                    samplerInfo.sampler = drawCommand.pixelSampler;

                                    std::array<VkWriteDescriptorSet, 3> writes{};
                                    uint32_t writeCount = 0;

                                    if (bufferInfo.buffer != VK_NULL_HANDLE)
                                    {
                                        auto &write = writes[writeCount++];
                                        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                        write.dstSet = descriptorSet;
                                        write.dstBinding = 0;
                                        write.descriptorCount = 1;
                                        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                                        write.pBufferInfo = &bufferInfo;
                                    }

                                    if (sampledImageInfo.imageView != VK_NULL_HANDLE)
                                    {
                                        auto &write = writes[writeCount++];
                                        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                        write.dstSet = descriptorSet;
                                        write.dstBinding = 1;
                                        write.descriptorCount = 1;
                                        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                                        write.pImageInfo = &sampledImageInfo;
                                    }

                                    if (samplerInfo.sampler != VK_NULL_HANDLE)
                                    {
                                        auto &write = writes[writeCount++];
                                        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                                        write.dstSet = descriptorSet;
                                        write.dstBinding = 2;
                                        write.descriptorCount = 1;
                                        write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                                        write.pImageInfo = &samplerInfo;
                                    }

                                    if (writeCount > 0)
                                    {
                                        vkUpdateDescriptorSets(device, writeCount, writes.data(), 0, nullptr);
                                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, uiGraphicsPipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
                                    }
                                }
                                else
                                {
                                    if (!isUiDraw)
                                    {
                                        ++sceneDescriptorSkips;
                                    }
                                    endRenderPassForCurrentTarget();
                                    continue;
                                }
                            }
                        }
                        else if (descriptorSetLayout != VK_NULL_HANDLE && descriptorPool != VK_NULL_HANDLE && graphicsPipelineLayout != VK_NULL_HANDLE)
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
                                    VkSampler sampler = drawCommand.pixelResourceSamplers[resourceSlot];
                                    Buffer *resourceBuffer = drawCommand.pixelResourceBuffers[resourceSlot];

                                    if (resourceSlot == 0)
                                    {
                                        if (imageView == VK_NULL_HANDLE)
                                        {
                                            imageView = drawCommand.pixelImageView;
                                        }

                                        if (sampler == VK_NULL_HANDLE)
                                        {
                                            sampler = drawCommand.pixelSampler;
                                        }
                                    }

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
                                        sampledImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
                                        sampler = drawCommand.pixelSampler;
                                    }

                                    if (sampler == VK_NULL_HANDLE)
                                    {
                                        sampler = drawCommand.pixelResourceSamplers[0];
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
                                    if (pixelConstantBuffer && pixelConstantBuffer->buffer != VK_NULL_HANDLE)
                                    {
                                        auto &pixelConstantBufferInfo = bufferInfos[bufferInfoCount++];
                                        pixelConstantBufferInfo.buffer = pixelConstantBuffer->buffer;
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
                                    if (!vertexConstantBuffer || vertexConstantBuffer->buffer == VK_NULL_HANDLE)
                                    {
                                        continue;
                                    }

                                    auto &vertexConstantBufferInfo = bufferInfos[bufferInfoCount++];
                                    vertexConstantBufferInfo.buffer = vertexConstantBuffer->buffer;
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
                                }
                            }
                            else
                            {
                                if (!isUiDraw)
                                {
                                    ++sceneDescriptorSkips;
                                }
                                endRenderPassForCurrentTarget();
                                continue;
                            }
                        }

                        if (drawCommand.indexed)
                        {
                            vkCmdDrawIndexed(commandBuffer, drawCommand.indexCount, std::max(drawCommand.instanceCount, 1u), 0, drawCommand.firstVertex, drawCommand.firstInstance);
                            if (!isUiDraw)
                            {
                                ++sceneDrawCallsIssued;
                                if (drawToBackBuffer)
                                {
                                    ++sceneBackBufferDrawCalls;
                                    bool hasPixelImage = (drawCommand.pixelImageView != VK_NULL_HANDLE);
                                    if (!hasPixelImage)
                                    {
                                        hasPixelImage = std::any_of(
                                            std::begin(drawCommand.pixelResourceImageViews),
                                            std::end(drawCommand.pixelResourceImageViews),
                                            [](VkImageView imageView) { return imageView != VK_NULL_HANDLE; });
                                    }
                                    if (!hasPixelImage)
                                    {
                                        ++sceneBackBufferMissingImageDrawCalls;
                                    }
                                }
                                else
                                {
                                    ++sceneOffscreenDrawCalls;
                                }
                            }
                        }
                        else if (drawCommand.vertexCount > 0)
                        {
                            vkCmdDraw(commandBuffer, drawCommand.vertexCount, std::max(drawCommand.instanceCount, 1u), static_cast<uint32_t>(drawCommand.firstVertex), drawCommand.firstInstance);
                            if (!isUiDraw)
                            {
                                ++sceneDrawCallsIssued;
                                if (drawToBackBuffer)
                                {
                                    ++sceneBackBufferDrawCalls;
                                    bool hasPixelImage = (drawCommand.pixelImageView != VK_NULL_HANDLE);
                                    if (!hasPixelImage)
                                    {
                                        hasPixelImage = std::any_of(
                                            std::begin(drawCommand.pixelResourceImageViews),
                                            std::end(drawCommand.pixelResourceImageViews),
                                            [](VkImageView imageView) { return imageView != VK_NULL_HANDLE; });
                                    }
                                    if (!hasPixelImage)
                                    {
                                        ++sceneBackBufferMissingImageDrawCalls;
                                    }
                                }
                                else
                                {
                                    ++sceneOffscreenDrawCalls;
                                }
                            }
                        }

                        endRenderPassForCurrentTarget();
                    }
                }

                constexpr bool allowSceneFallbackCopy = false;
                if (allowSceneFallbackCopy && hasDrawCommands && sceneOffscreenCopySourceImage != VK_NULL_HANDLE)
                {
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
                        VK_FILTER_NEAREST);

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
                    sceneFallbackCopyPerformed = true;
                }

                transitionSwapChainImage(imageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

                if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
                {
                    getContext()->log(Gek::Context::Error, "Vulkan failed to record command buffer");
                    pendingDrawCommands.clear();
                    return;
                }

                VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
                VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TRANSFER_BIT };
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
                {
                    std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                    submitResult = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);
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
                {
                    std::lock_guard<std::mutex> queueLock(getQueueSubmitMutex());
                    presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
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
                const uint32_t uiCommandCount = (totalCommandCount >= sceneCommandCount) ? (totalCommandCount - sceneCommandCount) : 0u;
                debugOverlayText = std::format(
                    "VK f={} total={} ui={} scene={} draws={} off={} back={} descSkips={} pipeSkips={} offSkips={} devLost={}",
                    presentFrameIndex,
                    totalCommandCount,
                    uiCommandCount,
                    sceneCommandCount,
                    sceneDrawCallsIssued,
                    sceneOffscreenDrawCalls,
                    sceneBackBufferDrawCalls,
                    sceneDescriptorSkips,
                    scenePipelineSkips,
                    sceneOffscreenInvalidSkips,
                    static_cast<uint32_t>(deviceLost));
                if ((presentFrameIndex % 240u) == 0u)
                {
                    getContext()->log(
                        Gek::Context::Info,
                        "Vulkan scene stats frame={} totalCommands={} uiCommands={} sceneCommands={} sceneDraws={} sceneBackBufferDraws={} sceneOffscreenDraws={} sceneBackBufferMissingImageDraws={} sceneFallbackCopy={} offscreenSkips={} pipelineSkips={} descriptorSkips={} contextDrawAttempts={} skipMissingProgram={} skipMissingVB={} skipMissingIB={} skipZeroCount={} deferredFinishCalls={} deferredExecuteCalls={} deferredExecutedCommands={} deferredContextQueues={} deferredCommandLists={} pendingEnqueues={} deferredEnqueues={}",
                        presentFrameIndex,
                        totalCommandCount,
                        uiCommandCount,
                        sceneCommandCount,
                        sceneDrawCallsIssued,
                        sceneBackBufferDrawCalls,
                        sceneOffscreenDrawCalls,
                        sceneBackBufferMissingImageDrawCalls,
                        static_cast<uint32_t>(sceneFallbackCopyPerformed),
                        sceneOffscreenInvalidSkips,
                        scenePipelineSkips,
                        sceneDescriptorSkips,
                        static_cast<uint32_t>(contextDrawCallAttempts),
                        static_cast<uint32_t>(contextDrawSkipsMissingProgram),
                        static_cast<uint32_t>(contextDrawSkipsMissingVertexBuffer),
                        static_cast<uint32_t>(contextDrawSkipsMissingIndexBuffer),
                        static_cast<uint32_t>(contextDrawSkipsZeroCount),
                        static_cast<uint32_t>(deferredFinishCalls),
                        static_cast<uint32_t>(deferredExecuteCalls),
                        static_cast<uint32_t>(deferredExecutedCommandCount),
                        static_cast<uint32_t>(deferredContextDrawCommands.size()),
                        static_cast<uint32_t>(deferredCommandLists.size()),
                        static_cast<uint32_t>(pendingCommandEnqueueCount),
                        static_cast<uint32_t>(deferredCommandEnqueueCount));
                }

                transientFrameConstantBufferSnapshotsInFlight = std::move(transientFrameConstantBufferSnapshotsPending);
                transientFrameConstantBufferSnapshotsPending.clear();
                pendingDrawCommands.clear();
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Render::Implementation
}; // namespace Gek