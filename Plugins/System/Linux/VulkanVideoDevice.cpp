#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/System/Window.hpp"
#include <algorithm>
#include <execution>
#include <memory>

namespace Gek
{
    namespace Vulkan
    {
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

        GEK_CONTEXT_USER(Device, Window *, Video::Device::Description)
            , public Video::Debug::Device
        {
            class Context
                : public Video::Device::Context
            {
                class ComputePipeline
                    : public Video::Device::Context::Pipeline
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
                    : public Video::Device::Context::Pipeline
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
                    : public Video::Device::Context::Pipeline
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
                    : public Video::Device::Context::Pipeline
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
            Video::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

        public:
            Device(Gek::Context *context, Window *window, Video::Device::Description deviceDescription)
                : ContextRegistration(context)
                , window(window)
            {
                defaultContext = std::make_unique<Context>();
            }

            ~Device(void)
            {
                setFullScreenState(false);

                backBuffer = nullptr;
                defaultContext = nullptr;
            }

            // Video::Debug::Device
            void * getDevice(void)
            {
                return nullptr;
            }

            // Video::Device
            Video::DisplayModeList getDisplayModeList(Video::Format format) const
            {
                Video::DisplayModeList displayModeList;
                return displayModeList;
            }

            void setFullScreenState(bool fullScreen)
            {
            }

            void setDisplayMode(const Video::DisplayMode &displayMode)
            {
            }

            void handleResize(void)
            {
            }

            Video::Target * const getBackBuffer(void)
            {
                return backBuffer.get();
            }

            Video::Device::Context * const getDefaultContext(void)
            {
                assert(defaultContext);

                return defaultContext.get();
            }

            Video::Device::ContextPtr createDeferredContext(void)
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

                std::cerr << "Unknown program pipline encountered";
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

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Vulkan
}; // namespace Gek