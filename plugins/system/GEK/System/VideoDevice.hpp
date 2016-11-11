/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: aba582126fd326535c2ce09055b12123f07428b2 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Sun Oct 23 19:59:14 2016 +0000 $
#pragma once

#include "GEK\Math\Common.hpp"
#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\SIMD\Vector4.hpp"
#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\JSON.hpp"
#include "GEK\Utility\Context.hpp"
#include <unordered_map>
#include <functional>
#include <memory>

namespace Gek
{
    namespace Video
    {
        GEK_ADD_EXCEPTION(CreationFailed);
        GEK_ADD_EXCEPTION(InitializationFailed);
        GEK_ADD_EXCEPTION(FeatureLevelNotSupported);
        GEK_ADD_EXCEPTION(UnsupportedOperation);
        GEK_ADD_EXCEPTION(OperationFailed);
        GEK_ADD_EXCEPTION(CreateObjectFailed);
        GEK_ADD_EXCEPTION(ProgramCompilationFailed);
        GEK_ADD_EXCEPTION(InvalidParameter);
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

        Format getFormat(const String &format);

        struct DisplayMode
        {
            enum class AspectRatio : uint8_t
            {
                Unknown = 0,
                _4x3,
                _16x9,
                _16x10,
            };

            uint32_t width = 0;
            uint32_t height = 0;
            Format format = Video::Format::Unknown;
            AspectRatio aspectRatio = AspectRatio::Unknown;
            struct
            {
                uint32_t numerator = 0;
                uint32_t denominator = 0;
            } refreshRate;
        };

        using DisplayModeList = std::vector<DisplayMode>;

        enum class PipelineType : uint8_t
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
            Math::Float2 position = Math::Float2::Zero;
            Math::Float2 size = Math::Float2::Zero;
            float nearClip = 0.0f;
            float farClip = 0.0f;

			ViewPort(void) = default;

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

			FillMode fillMode = FillMode::Solid;
            CullMode cullMode = CullMode::Back;
            bool frontCounterClockwise = false;
            uint32_t depthBias = 0;
            float depthBiasClamp = 0.0f;
            float slopeScaledDepthBias = 0.0f;
            bool depthClipEnable = true;
            bool scissorEnable = false;
            bool multisampleEnable = false;
            bool antialiasedLineEnable = false;

            void load(const JSON::Object &object);
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

                Operation failOperation = Operation::Keep;
                Operation depthFailOperation = Operation::Keep;
                Operation passOperation = Operation::Keep;
                ComparisonFunction comparisonFunction = ComparisonFunction::Always;

                void load(const JSON::Object &object);
            };

            bool enable = false;
            Write writeMask = Write::All;
            ComparisonFunction comparisonFunction = ComparisonFunction::Always;
            bool stencilEnable = false;
            uint8_t stencilReadMask = 0xFF;
            uint8_t stencilWriteMask = 0xFF;
            StencilStateInformation stencilFrontState;
            StencilStateInformation stencilBackState;

            void load(const JSON::Object &object);
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

			bool enable = false;
            Source colorSource = Source::One;
            Source colorDestination = Source::One;
            Operation colorOperation = Operation::Add;
            Source alphaSource = Source::One;
            Source alphaDestination = Source::One;
            Operation alphaOperation = Operation::Add;
            uint8_t writeMask = Mask::RGBA;

            void load(const JSON::Object &object);
        };

        struct UnifiedBlendStateInformation
            : public BlendStateInformation
        {
            bool alphaToCoverage = false;

            void load(const JSON::Object &object);
        };

        struct IndependentBlendStateInformation
        {
            bool alphaToCoverage = false;
            BlendStateInformation targetStates[8];

            void load(const JSON::Object &object);
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

			FilterMode filterMode = FilterMode::AllPoint;
            AddressMode addressModeU = AddressMode::Clamp;
            AddressMode addressModeV = AddressMode::Clamp;
            AddressMode addressModeW = AddressMode::Clamp;
            float mipLevelBias = 0.0f;
            uint32_t maximumAnisotropy = 1;
            ComparisonFunction comparisonFunction = ComparisonFunction::Never;
            Math::Float4 borderColor = Math::Float4(0.0f, 0.0f, 0.0f, 1.0f);
            float minimumMipLevel = 0.0f;
            float maximumMipLevel = Math::Infinity;

            void load(const JSON::Object &object);
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
            virtual ~Object(void) = default;

            virtual void setName(const wchar_t *name) = 0;
        };

        GEK_INTERFACE(Buffer)
            : virtual public Object
        {
            virtual ~Buffer(void) = default;
        
            virtual Video::Format getFormat(void) = 0;
            virtual uint32_t getStride(void) = 0;
            virtual uint32_t getCount(void) = 0;
        };

        GEK_INTERFACE(Texture)
            : virtual public Object
        {
            virtual ~Texture(void) = default;
        
            virtual Video::Format getFormat(void) = 0;
            virtual uint32_t getWidth(void) = 0;
            virtual uint32_t getHeight(void) = 0;
            virtual uint32_t getDepth(void) = 0;
        };

        GEK_INTERFACE(Target)
            : virtual public Texture
        {
            virtual ~Target(void) = default;
            
            virtual const Video::ViewPort &getViewPort(void) = 0;
        };

        GEK_INTERFACE(Device)
        {
			GEK_INTERFACE(Context)
            {
                GEK_INTERFACE(Pipeline)
                {
                    virtual ~Pipeline(void) = default;

                    virtual Video::PipelineType getType(void) const = 0;

					virtual void setProgram(Object *program) = 0;

                    virtual void setSamplerStateList(const std::vector<Object *> &samplerStateList, uint32_t firstStage) = 0;
                    virtual void setConstantBufferList(const std::vector<Buffer *> &constantBufferList, uint32_t firstStage) = 0;
                    virtual void setResourceList(const std::vector<Object *> &resourceList, uint32_t firstStage) = 0;
                    virtual void setUnorderedAccessList(const std::vector<Object *> &unorderedAccessList, uint32_t firstStage, uint32_t *countList = nullptr) = 0;

                    virtual void clearSamplerStateList(uint32_t count, uint32_t firstStage) = 0;
                    virtual void clearConstantBufferList(uint32_t count, uint32_t firstStage) = 0;
                    virtual void clearResourceList(uint32_t count, uint32_t firstStage) = 0;
                    virtual void clearUnorderedAccessList(uint32_t count, uint32_t firstStage) = 0;
                };

                virtual ~Context(void) = default;

                virtual Pipeline * const computePipeline(void) = 0;
                virtual Pipeline * const vertexPipeline(void) = 0;
                virtual Pipeline * const geometryPipeline(void) = 0;
                virtual Pipeline * const pixelPipeline(void) = 0;

                virtual void generateMipMaps(Texture *texture) = 0;

                virtual void clearState(void) = 0;
                virtual void clearUnorderedAccess(Object *object, const Math::SIMD::Float4 &value) = 0;
                virtual void clearUnorderedAccess(Object *object, const uint32_t value[4]) = 0;
                virtual void clearRenderTarget(Target *renderTarget, const Math::Float4 &clearColor) = 0;
                virtual void clearDepthStencilTarget(Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

                virtual void clearRenderTargetList(uint32_t count, bool depthBuffer) = 0;
                virtual void clearIndexBuffer(void) = 0;
                virtual void clearVertexBufferList(uint32_t count, uint32_t firstSlot) = 0;

                virtual void setViewportList(const std::vector<Video::ViewPort> &viewPortList) = 0;
                virtual void setScissorList(const std::vector<Math::UInt4> &rectangleList) = 0;
                virtual void setRenderTargetList(const std::vector<Target *> &renderTargetList, Object *depthBuffer) = 0;

                virtual void setRenderState(Object *renderState) = 0;
                virtual void setDepthState(Object *depthState, uint32_t stencilReference) = 0;
                virtual void setBlendState(Object *blendState, const Math::Float4 &blendFactor, uint32_t sampleMask) = 0;

                virtual void setInputLayout(Object *inputLayout) = 0;
                virtual void setIndexBuffer(Buffer *indexBuffer, uint32_t offset) = 0;
                virtual void setVertexBufferList(const std::vector<Buffer *> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList = nullptr) = 0;
                virtual void setPrimitiveType(Video::PrimitiveType type) = 0;

                virtual void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex) = 0;
                virtual void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;
                virtual void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
                virtual void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
                virtual void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;

                virtual ObjectPtr finishCommandList(void) = 0;
            };

            virtual ~Device(void) = default;

            virtual DisplayModeList getDisplayModeList(Video::Format format) const = 0;

            virtual void setFullScreenState(bool fullScreen) = 0;
            virtual void setDisplayMode(const DisplayMode &displayMode) = 0;
            virtual void handleResize(void) = 0;

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

            template <typename TYPE>
            void mapBuffer(Buffer *buffer, TYPE *&data, Video::Map mapping = Video::Map::WriteDiscard)
            {
                mapBuffer(buffer, (void *&)data, mapping);
            }

            virtual void mapBuffer(Buffer *buffer, void *&data, Video::Map mapping = Video::Map::WriteDiscard) = 0;
            virtual void unmapBuffer(Buffer *buffer) = 0;

            virtual void updateResource(Object *buffer, const void *data) = 0;
            virtual void copyResource(Object *destination, Object *source) = 0;

			virtual ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize) = 0;

            virtual std::vector<uint8_t> compileProgram(PipelineType pipelineType, const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction) = 0;
            virtual ObjectPtr createProgram(PipelineType pipelineType, const void *compiledData, uint32_t compiledSize) = 0;

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
