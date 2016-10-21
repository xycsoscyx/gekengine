#pragma warning(disable : 4005)

#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
#include <Windows.h>

namespace Gek
{
    namespace Vulkan
    {
        class CommandList
            : public Video::Object
        {
        public:

        public:
            CommandList(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class RenderState
            : public Video::Object
        {
        public:

        public:
            RenderState(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class DepthState
            : public Video::Object
        {
        public:

        public:
            DepthState(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class BlendState
            : public Video::Object
        {
        public:

        public:
            BlendState(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class SamplerState
            : public Video::Object
        {
        public:

        public:
            SamplerState(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class Event
            : public Video::Object
        {
        public:

        public:
            Event(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

		class InputLayout
			: public Video::Object
		{
		public:

		public:
			InputLayout(void)
			{
			}

			void setName(const wchar_t *name)
			{
			}
		};

		class ComputeProgram
			: public Video::Object
		{
		public:

		public:
			ComputeProgram(void)
			{
			}

			void setName(const wchar_t *name)
			{
			}
		};

		class VertexProgram
            : public Video::Object
        {
        public:

        public:
            VertexProgram(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class GeometryProgram
            : public Video::Object
        {
        public:

        public:
            GeometryProgram(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class PixelProgram
            : public Video::Object
        {
        public:

        public:
            PixelProgram(void)
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class Resource
        {
        public:

        public:
            Resource(void)
            {
            }

            virtual ~Resource(void) = default;
        };

        class ShaderResourceView
        {
        public:

        public:
            ShaderResourceView(void)
            {
            }

            virtual ~ShaderResourceView(void) = default;
        };

        class UnorderedAccessView
        {
        public:

        public:
            UnorderedAccessView(void)
            {
            }

            virtual ~UnorderedAccessView(void) = default;
        };

        class RenderTargetView
        {
        public:

        public:
            RenderTargetView(void)
            {
            }

            virtual ~RenderTargetView(void) = default;
        };

        class Buffer
            : public Video::Buffer
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            Video::Format format;
            uint32_t stride;
            uint32_t count;

        public:
            Buffer(Video::Format format, uint32_t stride, uint32_t count)
                : Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
                , format(format)
                , stride(stride)
                , count(count)
            {
            }

            virtual ~Buffer(void) = default;

            void setName(const wchar_t *name)
            {
            }

            // Video::Buffer
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getStride(void)
            {
                return stride;
            }

            uint32_t getCount(void)
            {
                return stride;
            }
        };

        class Texture
            : virtual public Video::Texture
        {
        public:
            Video::Format format;
            uint32_t width;
            uint32_t height;
            uint32_t depth;

        public:
            Texture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : format(format)
                , width(width)
                , height(height)
                , depth(depth)
            {
            }

            virtual ~Texture(void) = default;

            // Video::Texture
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getWidth(void)
            {
                return width;
            }

            uint32_t getHeight(void)
            {
                return height;
            }

            uint32_t getDepth(void)
            {
                return depth;
            }
        };

        class ViewTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            ViewTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Texture(format, width, height, depth)
                , Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }

            void setName(const wchar_t *name)
            {
            }
        };

        class Target
            : virtual public Video::Target
        {
        public:
            Video::Format format;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            Video::ViewPort viewPort;

        public:
            Target(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : format(format)
                , width(width)
                , height(height)
                , depth(depth)
                , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(width), float(height)), 0.0f, 1.0f)
            {
            }

            virtual ~Target(void) = default;

            // Video::Texture
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getWidth(void)
            {
                return width;
            }

            uint32_t getHeight(void)
            {
                return height;
            }

            uint32_t getDepth(void)
            {
                return depth;
            }

            // Video::Target
            const Video::ViewPort &getViewPort(void)
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
            TargetTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Target(format, width, height, depth)
                , Resource()
                , RenderTargetView()
            {
            }

            virtual ~TargetTexture(void) = default;

            virtual void setName(const wchar_t *name)
            {
            }
        };

        class TargetViewTexture
            : virtual public TargetTexture
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            TargetViewTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : TargetTexture(format, width, height, depth)
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }

            virtual ~TargetViewTexture(void) = default;

            void setName(const wchar_t *name)
            {
            }
        };

        class DepthTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:

        public:
            DepthTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Texture(format, width, height, depth)
                , Resource()
                , ShaderResourceView()
                , UnorderedAccessView()
            {
            }

            virtual ~DepthTexture(void) = default;

            void setName(const wchar_t *name)
            {
            }
        };

        GEK_CONTEXT_USER(Device, HWND, Video::Format, String)
            , public Video::Device
        {
            class Context
            : public Video::Device::Context
        {
            class ComputePipeline
                : public Video::Device::Context::Pipeline
            {
            private:

            public:
                ComputePipeline(void)
                {
                }

                // Video::Pipeline
                Video::PipelineType getType(void) const
                {
                    return Video::PipelineType::Compute;
                }

                void setProgram(Video::Object *program)
                {
                }

                void setSamplerStateList(const std::vector<Video::Object *> &samplerStateList, uint32_t firstStage)
                {
                }

                void setConstantBufferList(const std::vector<Video::Buffer *> &constantBufferList, uint32_t firstStage)
                {
                }

                void setResourceList(const std::vector<Video::Object *> &resourceList, uint32_t firstStage)
                {
                }

                void setUnorderedAccessList(const std::vector<Video::Object *> &unorderedAccessList, uint32_t firstStage, uint32_t *countList)
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
                VertexPipeline(void)
                {
                }

                // Video::Pipeline
                Video::PipelineType getType(void) const
                {
                    return Video::PipelineType::Vertex;
                }

                void setProgram(Video::Object *program)
                {
                }

                void setSamplerStateList(const std::vector<Video::Object *> &samplerStateList, uint32_t firstStage)
                {
                }

                void setConstantBufferList(const std::vector<Video::Buffer *> &constantBufferList, uint32_t firstStage)
                {
                }

                void setResourceList(const std::vector<Video::Object *> &resourceList, uint32_t firstStage)
                {
                }

                void setUnorderedAccessList(const std::vector<Video::Object *> &unorderedAccessList, uint32_t firstStage, uint32_t *countList)
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

            class GeometryPipeline
                : public Video::Device::Context::Pipeline
            {
            private:

            public:
                GeometryPipeline(void)
                {
                }

                // Video::Pipeline
                Video::PipelineType getType(void) const
                {
                    return Video::PipelineType::Geometry;
                }

                void setProgram(Video::Object *program)
                {
                }

                void setSamplerStateList(const std::vector<Video::Object *> &samplerStateList, uint32_t firstStage)
                {
                }

                void setConstantBufferList(const std::vector<Video::Buffer *> &constantBufferList, uint32_t firstStage)
                {
                }

                void setResourceList(const std::vector<Video::Object *> &resourceList, uint32_t firstStage)
                {
                }

                void setUnorderedAccessList(const std::vector<Video::Object *> &unorderedAccessList, uint32_t firstStage, uint32_t *countList)
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

            class PixelPipeline
                : public Video::Device::Context::Pipeline
            {
            private:

            public:
                PixelPipeline(void)
                {
                }

                // Video::Pipeline
                Video::PipelineType getType(void) const
                {
                    return Video::PipelineType::Pixel;
                }

                void setProgram(Video::Object *program)
                {
                }

                void setSamplerStateList(const std::vector<Video::Object *> &samplerStateList, uint32_t firstStage)
                {
                }

                void setConstantBufferList(const std::vector<Video::Buffer *> &constantBufferList, uint32_t firstStage)
                {
                }

                void setResourceList(const std::vector<Video::Object *> &resourceList, uint32_t firstStage)
                {
                }

                void setUnorderedAccessList(const std::vector<Video::Object *> &unorderedAccessList, uint32_t firstStage, uint32_t *countList)
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
            Device *device;
            PipelinePtr computeSystemHandler;
            PipelinePtr vertexSystemHandler;
            PipelinePtr geomtrySystemHandler;
            PipelinePtr pixelSystemHandler;

        public:
            Context(Device *device)
                : device(device)
                , computeSystemHandler(new ComputePipeline())
                , vertexSystemHandler(new VertexPipeline())
                , geomtrySystemHandler(new GeometryPipeline())
                , pixelSystemHandler(new PixelPipeline())
            {
                GEK_REQUIRE(device);
                GEK_REQUIRE(computeSystemHandler);
                GEK_REQUIRE(vertexSystemHandler);
                GEK_REQUIRE(geomtrySystemHandler);
                GEK_REQUIRE(pixelSystemHandler);
            }

            // Video::Context
            Pipeline * const computePipeline(void)
            {
                GEK_REQUIRE(computeSystemHandler);

                return computeSystemHandler.get();
            }

            Pipeline * const vertexPipeline(void)
            {
                GEK_REQUIRE(vertexSystemHandler);

                return vertexSystemHandler.get();
            }

            Pipeline * const geometryPipeline(void)
            {
                GEK_REQUIRE(geomtrySystemHandler);

                return geomtrySystemHandler.get();
            }

            Pipeline * const pixelPipeline(void)
            {
                GEK_REQUIRE(pixelSystemHandler);

                return pixelSystemHandler.get();
            }

            void generateMipMaps(Video::Texture *texture)
            {
            }

            void clearState(void)
            {
            }

            void clearResource(Video::Object *object, const Math::Float4 &value)
            {
            }

            void clearUnorderedAccess(Video::Object *object, const Math::Float4 &value)
            {
            }

            void clearUnorderedAccess(Video::Object *object, const uint32_t value[4])
            {
            }

            void clearRenderTarget(Video::Target *renderTarget, const Math::Color &clearColor)
            {
            }

            void clearDepthStencilTarget(Video::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
            {
            }

            void clearRenderTargetList(uint32_t count, bool depthBuffer)
            {
            }

            void clearIndexBuffer(void)
            {
            }

            void clearVertexBufferList(uint32_t count, uint32_t firstSlot)
            {
            }

            void setViewportList(const std::vector<Video::ViewPort> &viewPortList)
            {
            }

            void setScissorList(const std::vector<Video::ScissorBox> &rectangleList)
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

            void setBlendState(Video::Object *blendState, const Math::Color &blendFactor, uint32_t mask)
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
                return std::make_shared<CommandList>();
            }
        };

        public:
            HWND window;

            Video::DisplayModeList displayModeList;

            Video::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

        public:
            Device(Gek::Context *context, HWND window, Video::Format format, String device)
                : ContextRegistration(context)
                , window(window)
            {
                throw Exception("TODO: Finish OpenGL Video Device");

                GEK_REQUIRE(window);

                defaultContext = std::make_shared<Context>(this);
            }

            ~Device(void)
            {
                setFullScreen(false);

                backBuffer = nullptr;
                defaultContext = nullptr;
            }

            // System
            void setFullScreen(bool fullScreen)
            {
            }

            const Video::DisplayModeList &getDisplayModeList(void) const
            {
                return displayModeList;
            }

            void setDisplayMode(uint32_t displayMode)
            {
            }

            void resize(void)
            {
            }

            const char * const getSemanticMoniker(Video::InputElement::Semantic semantic)
            {
                return nullptr;
            }

            Video::Target * const getBackBuffer(void)
            {
                if (!backBuffer)
                {
                    backBuffer = std::make_shared<TargetTexture>(Video::Format::Unknown, 0, 0, 1);
                }

                return backBuffer.get();
            }

            Video::Device::Context * const getDefaultContext(void)
            {
                GEK_REQUIRE(defaultContext);

                return defaultContext.get();
            }

            Video::Device::ContextPtr createDeferredContext(void)
            {
                return std::make_shared<Context>(this);
            }

            Video::ObjectPtr createEvent(void)
            {
                return std::make_shared<Event>();
            }

            void setEvent(Video::Object *event)
            {
            }

            bool isEventSet(Video::Object *event)
            {
                return false;
            }

            Video::ObjectPtr createRenderState(const Video::RenderStateInformation &renderState)
            {
                return std::make_shared<RenderState>();
            }

            Video::ObjectPtr createDepthState(const Video::DepthStateInformation &depthState)
            {
                return std::make_shared<DepthState>();
            }

            Video::ObjectPtr createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                return std::make_shared<BlendState>();
            }

            Video::ObjectPtr createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                return std::make_shared<BlendState>();
            }

            Video::ObjectPtr createSamplerState(const Video::SamplerStateInformation &samplerState)
            {
                return std::make_shared<SamplerState>();
            }

            Video::BufferPtr createBuffer(Video::Format format, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
            {
                return std::make_shared<Buffer>(format, stride, count);
            }

            Video::BufferPtr createBuffer(uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
            {
                return createBuffer(Video::Format::Unknown, stride, count, type, flags, data);
            }

            Video::BufferPtr createBuffer(Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
            {
                uint32_t stride = 0;//DirectX::FormatStrideList[static_cast<uint8_t>(format)];
                return createBuffer(format, stride, count, type, flags, data);
            }

            void mapBuffer(Video::Buffer *buffer, void *&data, Video::Map mapping)
            {
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


            Video::ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize)
            {
                return std::make_shared<InputLayout>();
            }

            Video::ObjectPtr createProgram(Video::PipelineType pipelineType, const void *compiledData, uint32_t compiledSize)
            {
                return std::make_shared<ComputeProgram>();
            }

            std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction)
            {
                return std::vector<uint8_t>();
            }

            Video::TexturePtr createTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags, const void *data)
            {
                if (flags & Video::TextureFlags::RenderTarget)
                {
                    return std::make_shared<TargetViewTexture>(format, width, height, depth);
                }
                else if (flags & Video::TextureFlags::DepthTarget)
                {
                    return std::make_shared<DepthTexture>(format, width, height, depth);
                }
                else
                {
                    return std::make_shared<ViewTexture>(format, width, height, depth);
                }
            }

            Video::TexturePtr loadTexture(const wchar_t *fileName, uint32_t flags)
            {
                return std::make_shared<ViewTexture>(Video::Format::Unknown, 0, 0, 0);
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
