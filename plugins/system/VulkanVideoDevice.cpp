#pragma warning(disable : 4005)

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
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
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}

					void setProgram(Video::Object *program)
                    {
                        dynamic_cast<ComputeProgram *>(program);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        dynamic_cast<SamplerState *>(samplerState);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        dynamic_cast<Buffer *>(buffer);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            dynamic_cast<ShaderResourceView *>(resourceList[resource]);
                        }
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        for (uint32_t unorderedAccess = 0; unorderedAccess < unorderedAccessCount; unorderedAccess++)
                        {
                            dynamic_cast<UnorderedAccessView *>(unorderedAccessList[unorderedAccess]);
                        }
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
					void setInputLayout(Video::Object *inputLayout)
					{
					}

					void setProgram(Video::Object *program)
                    {
                        dynamic_cast<VertexProgram *>(program);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        dynamic_cast<SamplerState *>(samplerState);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        dynamic_cast<Buffer *>(buffer);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            dynamic_cast<ShaderResourceView *>(resourceList[resource]);
                        }
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation();
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
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}

					void setProgram(Video::Object *program)
                    {
                        dynamic_cast<GeometryProgram *>(program);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        dynamic_cast<SamplerState *>(samplerState);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        dynamic_cast<Buffer *>(buffer);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            dynamic_cast<ShaderResourceView *>(resourceList[resource]);
                        }
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation();
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
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}

					void setProgram(Video::Object *program)
                    {
                        dynamic_cast<PixelProgram *>(program);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        dynamic_cast<SamplerState *>(samplerState);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        dynamic_cast<Buffer *>(buffer);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            dynamic_cast<ShaderResourceView *>(resourceList[resource]);
                        }
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        for (uint32_t unorderedAccess = 0; unorderedAccess < unorderedAccessCount; unorderedAccess++)
                        {
                            dynamic_cast<UnorderedAccessView *>(unorderedAccessList[unorderedAccess]);
                        }
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
                Device * const getDevice(void)
                {
                    return device;
                }

				void * const getDeviceContext(void)
				{
					return nullptr;
				}

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
                    dynamic_cast<ShaderResourceView *>(texture);
                }

                void clearState(void)
                {
                }

                void setViewports(const Video::ViewPort *viewPortList, uint32_t viewPortCount)
                {
                }

                void setScissorRect(const Shapes::Rectangle<uint32_t> *rectangleList, uint32_t rectangleCount)
                {
                }

                void clearResource(Video::Object *object, const Math::Float4 &value)
                {
                }

                void clearUnorderedAccess(Video::Object *object, const Math::Float4 &value)
                {
                    auto unorderedAccessView = dynamic_cast<UnorderedAccessView *>(object);
                }

                void clearUnorderedAccess(Video::Object *object, const uint32_t value[4])
                {
                    auto unorderedAccessView = dynamic_cast<UnorderedAccessView *>(object);
                }

                void clearRenderTarget(Video::Target *renderTarget, const Math::Color &clearColor)
                {
                    auto targetViewTexture = dynamic_cast<TargetViewTexture *>(renderTarget);
                }

                void clearDepthStencilTarget(Video::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    auto depthTexture = dynamic_cast<DepthTexture *>(depthBuffer);
                }

                void setRenderTargets(Video::Target **renderTargetList, uint32_t renderTargetCount, Video::Object *depthBuffer)
                {
                    for (uint32_t renderTarget = 0; renderTarget < renderTargetCount; renderTarget++)
                    {
                        dynamic_cast<RenderTargetView *>(renderTargetList[renderTarget]);
                    }
                }

                void setRenderState(Video::Object *renderState)
                {
                    dynamic_cast<RenderState *>(renderState);
                }

                void setDepthState(Video::Object *depthState, uint32_t stencilReference)
                {
                    dynamic_cast<DepthState *>(depthState);
                }

                void setBlendState(Video::Object *blendState, const Math::Color &blendFactor, uint32_t mask)
                {
                    dynamic_cast<BlendState *>(blendState);
                }

                void setVertexBuffer(uint32_t slot, Video::Buffer *vertexBuffer, uint32_t offset)
                {
                    dynamic_cast<Buffer *>(vertexBuffer);
                }

                void setIndexBuffer(Video::Buffer *indexBuffer, uint32_t offset)
                {
                    dynamic_cast<Buffer *>(indexBuffer);
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
            bool fullScreen;
            Video::Format format;
            uint32_t width;
            uint32_t height;

            Video::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

        public:
            Device(Gek::Context *context, HWND window, Video::Format format, String device)
                : ContextRegistration(context)
                , window(window)
                , fullScreen(fullScreen)
                , format(format)
            {
                throw Exception("TODO: Finish Vulkan Video Device");

                GEK_REQUIRE(window);

                RECT clientRectangle;
                GetClientRect(window, &clientRectangle);
                width = (clientRectangle.right - clientRectangle.left);
                height = (clientRectangle.bottom - clientRectangle.top);

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

            void setSize(uint32_t width, uint32_t height, Video::Format format)
            {
                this->width = width;
                this->height = height;
                this->format = format;
            }

            void resize(void)
            {
            }

			const char * const getSemanticMoniker(Video::InputElement::Semantic semantic)
			{
				return nullptr;
			}

			void * const getDevice(void)
			{
				return nullptr;
			}

			void * const getSwapChain(void)
            {
                return nullptr;
            }

            Video::Target * const getBackBuffer(void)
            {
                if (!backBuffer)
                {
                    backBuffer = std::make_shared<TargetTexture>(format, width, height, 1);
                }

                return backBuffer.get();
            }

            Video::Device::Context * const getDefaultContext(void)
            {
                GEK_REQUIRE(defaultContext);

                return defaultContext.get();
            }

            bool isFullScreen(void)
            {
                return fullScreen;
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
                dynamic_cast<Event *>(event);
            }

            bool isEventSet(Video::Object *event)
            {
                dynamic_cast<Event *>(event);
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

            void mapBuffer(Video::Buffer *buffer, void **data, Video::Map mapping)
            {
                dynamic_cast<Buffer *>(buffer);
            }

            void unmapBuffer(Video::Buffer *buffer)
            {
                dynamic_cast<Buffer *>(buffer);
            }

            void updateResource(Video::Object *object, const void *data)
            {
                dynamic_cast<Resource *>(object);
            }

            void copyResource(Video::Object *destination, Video::Object *source)
            {
                dynamic_cast<Resource *>(destination);
            }


			Video::ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize)
			{
				return std::make_shared<InputLayout>();
			}

			Video::ObjectPtr createComputeProgram(const void *compiledData, uint32_t compiledSize)
			{
				return std::make_shared<ComputeProgram>();
			}

			Video::ObjectPtr createVertexProgram(const void *compiledData, uint32_t compiledSize)
			{
				return std::make_shared<VertexProgram>();
			}

			Video::ObjectPtr createGeometryProgram(const void *compiledData, uint32_t compiledSize)
			{
				return std::make_shared<GeometryProgram>();
			}

			Video::ObjectPtr createPixelProgram(const void *compiledData, uint32_t compiledSize)
			{
				return std::make_shared<PixelProgram>();
			}
			
			std::vector<uint8_t> compileComputeProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
			{
				return std::vector<uint8_t>();
			}

			std::vector<uint8_t> compileVertexProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
			{
				return std::vector<uint8_t>();
			}

			std::vector<uint8_t> compileGeometryProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
			{
				return std::vector<uint8_t>();
			}

			std::vector<uint8_t> compilePixelProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
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
                dynamic_cast<CommandList *>(commandList);
            }

            void present(bool waitForVerticalSync)
            {
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Vulkan
}; // namespace Gek
