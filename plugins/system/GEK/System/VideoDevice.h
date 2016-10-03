#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Float2.h"
#include "GEK\Math\Float4.h"
#include "GEK\Shapes\Rectangle.h"
#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\Context.h"
#include <unordered_map>
#include <functional>
#include <memory>

namespace Gek
{
    namespace Video
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(CreationFailed);
        GEK_ADD_EXCEPTION(InitializationFailed);
        GEK_ADD_EXCEPTION(FeatureLevelNotSupported);
        GEK_ADD_EXCEPTION(UnsupportedOperation);
        GEK_ADD_EXCEPTION(OperationFailed);
        GEK_ADD_EXCEPTION(CreateObjectFailed);
        GEK_ADD_EXCEPTION(ProgramCompilationFailed);
        GEK_ADD_EXCEPTION(InvalidParameters);
        GEK_ADD_EXCEPTION(InvalidFileType);
        GEK_ADD_EXCEPTION(LoadFileFailed);

        enum class Format : uint8_t
        {
            Unknown = 0,

            R32G32B32A32_FLOAT,
            R16G16B16A16_FLOAT,
            R32G32B32_FLOAT,
            R11G11B10_FLOAT,
            R32G32_FLOAT,
            R16G16_FLOAT,
            R32_FLOAT,
            R16_FLOAT,

            R32G32B32A32_UINT,
            R16G16B16A16_UINT,
            R10G10B10A2_UINT,
            R8G8B8A8_UINT,
            R32G32B32_UINT,
            R32G32_UINT,
            R16G16_UINT,
            R8G8_UINT,
            R32_UINT,
            R16_UINT,
            R8_UINT,

            R32G32B32A32_INT,
            R16G16B16A16_INT,
            R8G8B8A8_INT,
            R32G32B32_INT,
            R32G32_INT,
            R16G16_INT,
            R8G8_INT,
            R32_INT,
            R16_INT,
            R8_INT,

            R16G16B16A16_UNORM,
            R10G10B10A2_UNORM,
            R8G8B8A8_UNORM,
            R8G8B8A8_UNORM_SRGB,
            R16G16_UNORM,
            R8G8_UNORM,
            R16_UNORM,
            R8_UNORM,

            R16G16B16A16_NORM,
            R8G8B8A8_NORM,
            R16G16_NORM,
            R8G8_NORM,
            R16_NORM,
            R8_NORM,

            D32_FLOAT_S8X24_UINT,
            D24_UNORM_S8_UINT,

            D32_FLOAT,
            D16_UNORM,

            Count,
        };

        struct DisplayMode
        {
            enum class AspectRatio : uint8_t
            {
                Unknown = 0,
                _4x3,
                _16x9,
                _16x10,
            };

            uint32_t width;
            uint32_t height;
            AspectRatio aspectRatio;
            struct
            {
                uint32_t numerator;
                uint32_t denominator;
            } refreshRate;
        };

        using DisplayModeList = std::vector<DisplayMode>;

        enum class ProgramType : uint8_t
        {
            Compute = 0,
            Vertex,
            Geometry,
            Pixel,
        };

        enum class ComparisonFunction : uint8_t
        {
            Always = 0,
            Never,
            Equal,
            NotEqual,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
        };

        enum class PrimitiveType : uint8_t
        {
            PointList = 0,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip,
        };

        enum class Map : uint8_t
        {
            Read = 0,
            Write,
            ReadWrite,
            WriteDiscard,
            WriteNoOverwrite,
        };

        enum class BufferType : uint8_t
        {
            Raw = 0,
            Vertex,
            Index,
            Constant,
            Structured,
        };

        namespace BufferFlags
        {
            enum
            {
                Staging = 1 << 0,
                Mappable = 1 << 1,
                Resource = 1 << 2,
                UnorderedAccess = 1 << 3,
                Counter = 1 << 4,
            };
        }; // BufferFlags

        namespace TextureFlags
        {
            enum
            {
                RenderTarget = 1 << 0,
                DepthTarget = 1 << 1,
                Resource = 1 << 2,
                UnorderedAccess = 1 << 3,
            };
        }; // TextureFlags

        namespace TextureLoadFlags
        {
            enum
            {
                sRGB = 1 << 0,
            };
        }; // TextureLoadFlags

        namespace ClearFlags
        {
            enum
            {
                Depth = 1 << 0,
                Stencil = 1 << 1,
            };
        }; // ClearFlags

        struct ViewPort
        {
            Math::Float2 position;
            Math::Float2 size;
            float nearClip;
            float farClip;

            ViewPort(void)
            {
            }

            ViewPort(const Math::Float2 &position, const Math::Float2 &size, float nearClip, float farClip)
                : position(position)
                , size(size)
                , nearClip(nearClip)
                , farClip(farClip)
            {
            }
        };

        struct RenderStateInformation
        {
			enum class FillMode : uint8_t
			{
				WireFrame = 0,
				Solid,
			};

			enum class CullMode : uint8_t
			{
				None = 0,
				Front,
				Back,
			};

			FillMode fillMode;
            CullMode cullMode;
            bool frontCounterClockwise;
            uint32_t depthBias;
            float depthBiasClamp;
            float slopeScaledDepthBias;
            bool depthClipEnable;
            bool scissorEnable;
            bool multisampleEnable;
            bool antialiasedLineEnable;

            RenderStateInformation(void)
                : fillMode(FillMode::Solid)
                , cullMode(CullMode::Back)
                , frontCounterClockwise(false)
                , depthBias(0)
                , depthBiasClamp(0.0f)
                , slopeScaledDepthBias(0.0f)
                , depthClipEnable(true)
                , scissorEnable(false)
                , multisampleEnable(false)
                , antialiasedLineEnable(false)
            {
            }
        };

        struct DepthStateInformation
        {
			enum class Write : uint8_t
			{
				Zero = 0,
				All,
			};

			struct StencilStateInformation
            {
				enum class Operation : uint8_t
				{
					Zero = 0,
					Keep,
					Replace,
					Invert,
					Increase,
					IncreaseSaturated,
					Decrease,
					DecreaseSaturated,
				};

				Operation failOperation;
                Operation depthFailOperation;
                Operation passOperation;
                ComparisonFunction comparisonFunction;

                StencilStateInformation(void)
                    : failOperation(Operation::Keep)
                    , depthFailOperation(Operation::Keep)
                    , passOperation(Operation::Keep)
                    , comparisonFunction(ComparisonFunction::Always)
                {
                }
            };

            bool enable;
            Write writeMask;
            ComparisonFunction comparisonFunction;
            bool stencilEnable;
            uint8_t stencilReadMask;
            uint8_t stencilWriteMask;
            StencilStateInformation stencilFrontState;
            StencilStateInformation stencilBackState;

            DepthStateInformation(void)
                : enable(false)
                , writeMask(Write::All)
                , comparisonFunction(ComparisonFunction::Always)
                , stencilEnable(false)
                , stencilReadMask(0xFF)
                , stencilWriteMask(0xFF)
            {
            }
        };

        struct BlendStateInformation
        {
			friend struct UnifiedBlendStateInformation;
			friend struct IndependentBlendStateInformation;

			enum class Source : uint8_t
			{
				Zero = 0,
				One,
				BlendFactor,
				InverseBlendFactor,
				SourceColor,
				InverseSourceColor,
				SourceAlpha,
				InverseSourceAlpha,
				SourceAlphaSaturated,
				DestinationColor,
				InverseDestinationColor,
				DestinationAlpha,
				InverseDestinationAlpha,
				SecondarySourceColor,
				InverseSecondarySourceColor,
				SecondarySourceAlpha,
				InverseSecondarySourceAlpha,
			};

			enum class Operation : uint8_t
			{
				Add = 0,
				Subtract,
				ReverseSubtract,
				Minimum,
				Maximum,
			};

			struct Mask
			{
				enum
				{
					R = 1 << 0,
					G = 1 << 1,
					B = 1 << 2,
					A = 1 << 3,
					RGB = (R | G | B),
					RGBA = (R | G | B | A),
				};
			}; // struct Mask

			bool enable;
            Source colorSource;
            Source colorDestination;
            Operation colorOperation;
            Source alphaSource;
            Source alphaDestination;
            Operation alphaOperation;
            uint8_t writeMask;

        private:
            BlendStateInformation(void)
                : enable(false)
                , colorSource(Source::One)
                , colorDestination(Source::One)
                , colorOperation(Operation::Add)
                , alphaSource(Source::One)
                , alphaDestination(Source::One)
                , alphaOperation(Operation::Add)
                , writeMask(Mask::RGBA)
            {
            }
        };

        struct UnifiedBlendStateInformation
            : public BlendStateInformation
        {
            bool alphaToCoverage;

            UnifiedBlendStateInformation(void)
                : alphaToCoverage(false)
            {
            }
        };

        struct IndependentBlendStateInformation
        {
            bool alphaToCoverage;
            BlendStateInformation targetStates[8];

            IndependentBlendStateInformation(void)
                : alphaToCoverage(false)
            {
            }
        };

        struct SamplerStateInformation
        {
			enum class FilterMode : uint8_t
			{
				AllPoint = 0,
				MinMagPointMipLinear,
				MinPointMAgLinearMipPoint,
				MinPointMagMipLinear,
				MinLinearMagMipPoint,
				MinLinearMagPointMipLinear,
				MinMagLinearMipPoint,
				AllLinear,
				Anisotropic,
			};

			enum class AddressMode : uint8_t
			{
				Clamp = 0,
				Wrap,
				Mirror,
				MirrorOnce,
				Border,
			};

			FilterMode filterMode;
            AddressMode addressModeU;
            AddressMode addressModeV;
            AddressMode addressModeW;
            float mipLevelBias;
            uint32_t maximumAnisotropy;
            ComparisonFunction comparisonFunction;
            Math::Color borderColor;
            float minimumMipLevel;
            float maximumMipLevel;

            SamplerStateInformation(void)
                : filterMode(FilterMode::AllPoint)
                , addressModeU(AddressMode::Clamp)
                , addressModeV(AddressMode::Clamp)
                , addressModeW(AddressMode::Clamp)
                , mipLevelBias(0.0f)
                , maximumAnisotropy(1)
                , comparisonFunction(ComparisonFunction::Never)
                , borderColor(0.0f, 0.0f, 0.0f, 1.0f)
                , minimumMipLevel(0.0f)
                , maximumMipLevel(Math::Infinity)
            {
            }
        };

        struct InputElement
        {
			enum class Source : uint8_t
			{
				Vertex = 0,
				Instance,
			};

			enum class Semantic : uint8_t
			{
				Position = 0,
				TexCoord,
				Tangent,
				BiTangent,
				Normal,
				Color,
				Count,
			};

			Video::Format format;
			Semantic semantic;
			Source source;
            uint32_t sourceIndex;

			InputElement(void)
                : format(Format::Unknown)
                , semantic(Semantic::TexCoord)
                , source(Source::Vertex)
                , sourceIndex(0)
            {
            }

			InputElement(Video::Format format, Semantic semantic, Source source = Source::Vertex, uint32_t sourceIndex = 0)
                : format(format)
                , semantic(semantic)
                , source(source)
                , sourceIndex(sourceIndex)
            {
            }
        };

        GEK_INTERFACE(Object)
        {
            virtual void setName(const wchar_t *name) = 0;
        };

        GEK_INTERFACE(Buffer)
            : virtual public Object
        {
            virtual Video::Format getFormat(void) = 0;
            virtual uint32_t getStride(void) = 0;
            virtual uint32_t getCount(void) = 0;
        };

        GEK_INTERFACE(Texture)
            : virtual public Object
        {
            virtual Video::Format getFormat(void) = 0;
            virtual uint32_t getWidth(void) = 0;
            virtual uint32_t getHeight(void) = 0;
            virtual uint32_t getDepth(void) = 0;
        };

        GEK_INTERFACE(Target)
            : virtual public Texture
        {
            virtual const Video::ViewPort &getViewPort(void) = 0;
        };

        GEK_INTERFACE(Device)
        {
			GEK_INTERFACE(Context)
            {
                GEK_INTERFACE(Pipeline)
                {
					virtual void setProgram(Object *program) = 0;
                    virtual void setSamplerState(Object *samplerState, uint32_t stage) = 0;
                    virtual void setConstantBuffer(Buffer *constantBuffer, uint32_t stage) = 0;

                    virtual void setResource(Object *resource, uint32_t firstStage) = 0;
                    virtual void setUnorderedAccess(Object *unorderedAccess, uint32_t firstStage, uint32_t count = 0) = 0;

                    virtual void setResourceList(Object **resourceList, uint32_t resourceCount, uint32_t firstStage) = 0;
                    virtual void setUnorderedAccessList(Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList = nullptr) = 0;
                };

                virtual Device * const getDevice(void) = 0;
				virtual void * const getDeviceContext(void) = 0;

                virtual Pipeline * const computePipeline(void) = 0;
                virtual Pipeline * const vertexPipeline(void) = 0;
                virtual Pipeline * const geometryPipeline(void) = 0;
                virtual Pipeline * const pixelPipeline(void) = 0;

                virtual void generateMipMaps(Texture *texture) = 0;

                virtual void clearState(void) = 0;

                virtual void setViewports(const Video::ViewPort *viewPortList, uint32_t viewPortCount) = 0;
                virtual void setScissorRect(const Shapes::Rectangle<uint32_t> *rectangleList, uint32_t rectangleCount) = 0;

                virtual void clearUnorderedAccess(Object *object, const Math::Float4 &value) = 0;
                virtual void clearUnorderedAccess(Object *object, const uint32_t value[4]) = 0;

                virtual void clearRenderTarget(Target *renderTarget, const Math::Color &clearColor) = 0;
                virtual void clearDepthStencilTarget(Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

                virtual void setRenderTargets(Target **renderTargetList, uint32_t renderTargetCount, Object *depthBuffer) = 0;

                virtual void setRenderState(Object *renderState) = 0;
                virtual void setDepthState(Object *depthState, uint32_t stencilReference) = 0;
                virtual void setBlendState(Object *blendState, const Math::Color &blendFactor, uint32_t sampleMask) = 0;

                virtual void setInputLayout(Object *inputLayout) = 0;
                virtual void setVertexBuffer(uint32_t slot, Buffer *vertexBuffer, uint32_t offset) = 0;
                virtual void setIndexBuffer(Buffer *indexBuffer, uint32_t offset) = 0;
                virtual void setPrimitiveType(Video::PrimitiveType type) = 0;

                virtual void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex) = 0;
                virtual void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;

                virtual void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
                virtual void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;

                virtual void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;

                virtual ObjectPtr finishCommandList(void) = 0;
            };

            virtual void setFullScreen(bool fullScreen) = 0;
            virtual const DisplayModeList &getDisplayModeList(void) const = 0;
            virtual void setDisplayMode(uint32_t displayMode) = 0;
            virtual void resize(void) = 0;

			virtual const char * const getSemanticMoniker(InputElement::Semantic semantic) = 0;

            virtual Target * const getBackBuffer(void) = 0;
            virtual Context * const getDefaultContext(void) = 0;

            virtual ContextPtr createDeferredContext(void) = 0;

            virtual ObjectPtr createEvent(void) = 0;
            virtual void setEvent(Object *event) = 0;
            virtual bool isEventSet(Object *event) = 0;

            virtual ObjectPtr createRenderState(const Video::RenderStateInformation &renderState) = 0;
            virtual ObjectPtr createDepthState(const Video::DepthStateInformation &depthState) = 0;
            virtual ObjectPtr createBlendState(const Video::UnifiedBlendStateInformation &blendState) = 0;
            virtual ObjectPtr createBlendState(const Video::IndependentBlendStateInformation &blendState) = 0;
            virtual ObjectPtr createSamplerState(const Video::SamplerStateInformation &samplerState) = 0;

            virtual TexturePtr createTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags, const void *data = nullptr) = 0;
            virtual TexturePtr loadTexture(const wchar_t *fileName, uint32_t flags) = 0;

            virtual BufferPtr createBuffer(uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData = nullptr) = 0;
            virtual BufferPtr createBuffer(Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const void *staticData = nullptr) = 0;
            virtual void mapBuffer(Buffer *buffer, void **data, Video::Map mapping = Video::Map::WriteDiscard) = 0;
            virtual void unmapBuffer(Buffer *buffer) = 0;

            virtual void updateResource(Object *buffer, const void *data) = 0;
            virtual void copyResource(Object *destination, Object *source) = 0;

			virtual ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize) = 0;

            virtual std::vector<uint8_t> compileProgram(ProgramType programType, const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction) = 0;
            virtual ObjectPtr createProgram(ProgramType programType, const void *compiledData, uint32_t compiledSize) = 0;

            virtual void executeCommandList(Object *commandList) = 0;

            virtual void present(bool waitForVerticalSync) = 0;
        };

        namespace Debug
        {
            GEK_INTERFACE(Device)
                : public Video::Device
            {
                virtual void *getDevice(void) = 0;
            };
        }; // namespace Debug
    }; // namespace Video
}; // namespace Gek
