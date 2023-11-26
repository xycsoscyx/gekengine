#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/System/Window.hpp"
#include <algorithm>
#include <execution>
#include <exception>
#include <memory>
#include <set>

#include <vulkan/vulkan.h>

#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

#undef Status

namespace Gek
{
    namespace Vulkan
    {
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
            return nullptr;
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

        public:
            BaseObject(void)
            {
            }

            virtual ~BaseObject(void)
            {
            }

            // Video::Object
            std::string_view getName(void) const
            {
                return "object";
            }
        };

        template <int UNIQUE, typename BASE = Video::Object>
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

            // Video::Object
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


            // Video::Object
            std::string_view getName(void) const
            {
                return description.name;
            }
        };

        using CommandList = BaseVideoObject<1>;
        using RenderState = DescribedVideoObject<Video::RenderState>;
        using DepthState = DescribedVideoObject<Video::DepthState>;
        using BlendState = DescribedVideoObject<Video::BlendState>;
        using SamplerState = DescribedVideoObject<Video::SamplerState>;
        using InputLayout = BaseVideoObject<2>;

        using Resource = BaseObject<2>;
        using ShaderResourceView = BaseObject<3>;
        using UnorderedAccessView = BaseObject<4>;
        using RenderTargetView = BaseObject<5>;

        class Query
            : public Video::Query
        {
        public:

        public:
            Query(void)
            {
            }

            virtual ~Query(void)
            {
            }

            // Video::Object
            std::string_view getName(void) const
            {
                return "query";
            }
        };

        class Buffer
            : public Video::Buffer
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            Video::Buffer::Description description;

        public:
            Buffer(const Video::Buffer::Description &description)
                : Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
                , description(description)
            {
            }

            virtual ~Buffer(void)
            {
            }

            // Video::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Video::Buffer
            const Video::Buffer::Description &getDescription(void) const
            {
                return description;
            }
        };

        class BaseTexture
        {
        public:
            Video::Texture::Description description;

        public:
            BaseTexture(const Video::Texture::Description &description)
                : description(description)
            {
            }
        };

        class Texture
            : virtual public Video::Texture
            , public BaseTexture
        {
        public:
            Texture(const Video::Texture::Description &description)
                : BaseTexture(description)
            {
            }

            virtual ~Texture(void) = default;

            // Video::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Video::Texture
            const Video::Texture::Description &getDescription(void) const
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
            ViewTexture(const Video::Texture::Description &description)
                : Texture(description)
                , Resource()
                , ShaderResourceView()
            {
            }
        };

        class UnorderedViewTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            UnorderedViewTexture(const Video::Texture::Description &description)
                : Texture(description)
                , Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }
        };

        class Target
            : virtual public Video::Target
            , public BaseTexture
        {
        public:
            Video::ViewPort viewPort;

        public:
            Target(const Video::Texture::Description &description)
                : BaseTexture(description)
                , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(description.width), float(description.height)), 0.0f, 1.0f)
            {
            }

            virtual ~Target(void) = default;

            // Video::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Video::Texture
            const Video::Texture::Description &getDescription(void) const
            {
                return description;
            }

            // Video::Target
            const Video::ViewPort &getViewPort(void) const
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
            TargetTexture(const Video::Texture::Description &description)
                : Target(description)
                , Resource()
                , RenderTargetView()
            {
            }

            virtual ~TargetTexture(void) = default;
        };

        class TargetViewTexture
            : virtual public TargetTexture
            , public ShaderResourceView
        {
        public:
            TargetViewTexture(const Video::Texture::Description &description)
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
            UnorderedTargetViewTexture(const Video::Texture::Description &description)
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
            DepthTexture(const Video::Texture::Description &description)
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
            : public Video::Program
        {
        public:
            Video::Program::Information information;

        public:
            Program(Video::Program::Information information)
                : information(information)
            {
            }

            virtual ~Program(void)
            {
            }

            // Video::Object
            std::string_view getName(void) const
            {
                return information.name;
            }

            // Video::Program
            Information const &getInformation(void) const
            {
                return information;
            }
        };

        using ComputeProgram = Program<1>;
        using VertexProgram = Program<2>;
        using GeometryProgram = Program<3>;
        using PixelProgram = Program<4>;

        GEK_CONTEXT_USER(Implementation, Window *, Render::Device::Description)
            , public Video::Debug::Device
        {
            class Context
                : public Render::Device::Context
            {
                class ComputePipeline
                    : public Render::Device::Context::Pipeline
                {
                private:

                public:
                    ComputePipeline()
                    {
                    }

                    // Video::Pipeline
                    Type getType(void) const
                    {
                        return Type::Compute;
                    }

                    void setProgram(Video::Program *program)
                    {
                    }

                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                    }

                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
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

                public:
                    VertexPipeline()
                    {
                    }

                    // Video::Pipeline
                    Type getType(void) const
                    {
                        return Type::Vertex;
                    }

                    void setProgram(Video::Program *program)
                    {
                    }

                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                    }

                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(false && "Vertex pipeline does not supported unordered access");
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
                        assert(false && "Vertex pipeline does not supported unordered access");
                    }
                };

                class GeometryPipeline
                    : public Render::Device::Context::Pipeline
                {
                private:

                public:
                    GeometryPipeline()
                    {
                    }

                    // Video::Pipeline
                    Type getType(void) const
                    {
                        return Type::Geometry;
                    }

                    void setProgram(Video::Program *program)
                    {
                    }

                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                    }

                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
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

                public:
                    PixelPipeline()
                    {
                    }

                    // Video::Pipeline
                    Type getType(void) const
                    {
                        return Type::Pixel;
                    }

                    void setProgram(Video::Program *program)
                    {
                    }

                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                    }

                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
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

            public:
    	        VkDevice device;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;

            public:
                Context()
                    : computeSystemHandler(new ComputePipeline())
                    , vertexSystemHandler(new VertexPipeline())
                    , geomtrySystemHandler(new GeometryPipeline())
                    , pixelSystemHandler(new PixelPipeline())
                {
                    assert(computeSystemHandler);
                    assert(vertexSystemHandler);
                    assert(geomtrySystemHandler);
                    assert(pixelSystemHandler);
                }

                // Video::Context
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

                void begin(Video::Query *query)
                {
                }

                void end(Video::Query *query)
                {
                }

                Video::Query::Status getData(Video::Query *query, void *data, size_t dataSize, bool waitUntilReady = false)
                {
                    return Video::Query::Status::Error;
                }

                void generateMipMaps(Video::Texture *texture)
                {
                }

                void resolveSamples(Video::Texture *destination, Video::Texture *source)
                {
                }

                void clearState(void)
                {
                }

                void setViewportList(const std::vector<Video::ViewPort> &viewPortList)
                {
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                }

                void clearResource(Video::Object *object, Math::Float4 const &value)
                {
                }

                void clearUnorderedAccess(Video::Object *object, Math::Float4 const &value)
                {
                }

                void clearUnorderedAccess(Video::Object *object, Math::UInt4 const &value)
                {
                }

                void clearRenderTarget(Video::Target *renderTarget, Math::Float4 const &clearColor)
                {
                }

                void clearDepthStencilTarget(Video::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                }

                void clearIndexBuffer(void)
                {
                }

                void clearVertexBufferList(uint32_t count, uint32_t firstSlot)
                {
                }

                void clearRenderTargetList(uint32_t count, bool depthBuffer)
                {
                }

                void setRenderTargetList(const std::vector<Video::Target *> &renderTargetList, Video::Object *depthBuffer)
                {
                }

                void setRenderState(Video::Object *renderState)
                {
                }

                void setDepthState(Video::Object *depthState, uint32_t stencilReference)
                {
                }

                void setBlendState(Video::Object *blendState, Math::Float4 const &blendFactor, uint32_t mask)
                {
                }

                void setInputLayout(Video::Object *inputLayout)
                {
                }

                void setIndexBuffer(Video::Buffer *indexBuffer, uint32_t offset)
                {
                }

                void setVertexBufferList(const std::vector<Video::Buffer *> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                }

                void setPrimitiveType(Video::PrimitiveType primitiveType)
                {
                }

                void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
                {
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                }

                Video::ObjectPtr finishCommandList(void)
                {
                    return std::make_unique<CommandList>();
                }
            };

        public:
            Window * window = nullptr;
            Render::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

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

        private:
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
                    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
                };

                if (!checkInstanceExtensionSupport(instanceExtensions))
                {
                    throw std::runtime_error("Required instances extensions not available");
                }

                VkInstanceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                createInfo.pApplicationInfo = &appInfo;
                createInfo.enabledExtensionCount = instanceExtensions.size();
                createInfo.ppEnabledExtensionNames = instanceExtensions.data();
                if (enableValidationLayer)
                {
                    createInfo.enabledLayerCount = validationLayers.size();
                    createInfo.ppEnabledLayerNames = validationLayers.data();
                }
                else
                {
                    createInfo.enabledLayerCount = 0;
                }

                VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
                if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) 
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

                VkDeviceCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
                createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
                createInfo.pQueueCreateInfos = queueCreateInfos.data();
                createInfo.pEnabledFeatures = &deviceFeatures;
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
                getContext()->log(Gek::Context::Info, "Vulkan logica device created");
            }

            void createSurface(void)
            {
                VkXlibSurfaceCreateInfoKHR createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
                createInfo.dpy = reinterpret_cast<Display *>(window->getWindowData(0));
                createInfo.window = *reinterpret_cast<::Window *>(window->getWindowData(1));
                if (vkCreateXlibSurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to create window surface!");
                }

                getContext()->log(Gek::Context::Info, "Vulkan surface created");
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
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
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
                        clientRectangle.size.x,
                        clientRectangle.size.y,
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

        public:
            Implementation(Gek::Context *context, Window *window, Render::Device::Description deviceDescription)
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

                defaultContext = std::make_unique<Context>();

                Video::Texture::Description description;
                backBuffer = std::make_unique<Target>(description);
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

            ~Implementation(void)
            {
                setFullScreenState(false);

                backBuffer = nullptr;
                defaultContext = nullptr;

                for (auto imageView : swapChainImageViews)
                {
                    vkDestroyImageView(device, imageView, nullptr);
                }

                vkDestroySwapchainKHR(device, swapChain, nullptr);
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

            // Video::Debug::Device
            void * getDevice(void)
            {
                return nullptr;
            }

            // Render::Device
            Video::DisplayModeList getDisplayModeList(Video::Format format) const
            {
                Video::DisplayModeList displayModeList;

                Video::DisplayMode displayMode(1600, 1200, Video::Format::Unknown);
                displayMode.refreshRate.numerator = 60;
                displayMode.refreshRate.denominator = 1;
                displayModeList.push_back(displayMode);
        
                for (auto displayMode : displayModeList)
                {
                    getContext()->log(Gek::Context::Info, "Display Mode: {} x {}", displayMode.width, displayMode.height);
                }

                return displayModeList;
            }

            void setFullScreenState(bool fullScreen)
            {
            }

            void setDisplayMode(const Video::DisplayMode &displayMode)
            {
                getContext()->log(Gek::Context::Info, "Setting display mode: {} x {}", displayMode.width, displayMode.height);
                window->resize(Math::Int2(displayMode.width, displayMode.height));
            }

            void handleResize(void)
            {
            }

            Video::Target * const getBackBuffer(void)
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
                return std::make_unique<Context>();
            }

            Video::QueryPtr createQuery(Video::Query::Type type)
            {
                return std::make_unique<Query>();
            }

            Video::RenderStatePtr createRenderState(Video::RenderState::Description const &description)
            {
                return std::make_unique<RenderState>(description);
            }

            Video::DepthStatePtr createDepthState(Video::DepthState::Description const &description)
            {
                return std::make_unique<DepthState>(description);
            }

            Video::BlendStatePtr createBlendState(Video::BlendState::Description const &description)
            {
                return std::make_unique<BlendState>(description);
            }

            Video::SamplerStatePtr createSamplerState(Video::SamplerState::Description const &description)
            {
                return std::make_unique<SamplerState>(description);
            }

            Video::BufferPtr createBuffer(const Video::Buffer::Description &description, const void *data)
            {
                return std::make_unique<Buffer>(description);
            }

            bool mapBuffer(Video::Buffer *buffer, void *&data, Video::Map mapping)
            {
                return false;
            }

            void unmapBuffer(Video::Buffer *buffer)
            {
            }

            void updateResource(Video::Object *object, const void *data)
            {
            }

            void copyResource(Video::Object *destination, Video::Object *source)
            {
            }

            std::string_view const getSemanticMoniker(Video::InputElement::Semantic semantic)
            {
                return SemanticNameList[static_cast<uint8_t>(semantic)];
            }

            Video::ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, Video::Program::Information const &information)
            {
                return std::make_unique<InputLayout>();
            }

            Video::Program::Information compileProgram(Video::Program::Type type, std::string_view name, FileSystem::Path const &debugPath, std::string_view uncompiledProgram, std::string_view entryFunction, std::function<bool(Video::IncludeType, std::string_view, void const **data, uint32_t *size)> &&onInclude)
            {
                assert(d3dDevice);

                Video::Program::Information information;
                information.type = type;
                return information;
            }

            template <class TYPE>
            Video::ProgramPtr createProgram(Video::Program::Information const &information)
            {
                return std::make_unique<TYPE>(information);
            }

            Video::ProgramPtr createProgram(Video::Program::Information const &information)
            {
                switch (information.type)
                {
                case Video::Program::Type::Compute:
                    return createProgram<ComputeProgram>(information);

                case Video::Program::Type::Vertex:
                    return createProgram<VertexProgram>(information);

                case Video::Program::Type::Geometry:
                    return createProgram<GeometryProgram>(information);

                case Video::Program::Type::Pixel:
                    return createProgram<PixelProgram>(information);
                };

                getContext()->log(Gek::Context::Error, "Unknown program pipline encountered");
                return nullptr;
            }

            Video::TexturePtr createTexture(const Video::Texture::Description &description, const void *data)
            {
                if (description.flags & Video::Texture::Flags::RenderTarget)
                {
                    return std::make_unique<UnorderedTargetViewTexture>(description);
                }
                else if (description.flags & Video::Texture::Flags::DepthTarget)
                {
                    return std::make_unique<DepthTexture>(description);
                }
                else
                {
                    return std::make_unique<UnorderedViewTexture>(description);
                }
            }

            Video::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                Video::Texture::Description description;
                return std::make_unique<ViewTexture>(description);
            }

            Video::TexturePtr loadTexture(void const *buffer, size_t size, uint32_t flags)
            {
                Video::Texture::Description description;
                return std::make_unique<ViewTexture>(description);
            }

            Texture::Description loadTextureDescription(FileSystem::Path const &filePath)
            {
                Texture::Description description;
                return description;
            }

            void executeCommandList(Video::Object *commandList)
            {
            }

            void present(bool waitForVerticalSync)
            {
            }
        };

        GEK_REGISTER_CONTEXT_USER(Implementation);
    }; // Vulkan
}; // namespace Gek