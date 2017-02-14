/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: aba582126fd326535c2ce09055b12123f07428b2 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Sun Oct 23 19:59:14 2016 +0000 $
#pragma once

#include "GEK/Math/Common.hpp"
#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Context.hpp"
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

        Format getFormat(String const &format);

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

			ViewPort(Math::Float2 const &position, Math::Float2 const &size, float nearClip, float farClip)
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
            size_t getHash(void) const;
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
                size_t getHash(void) const;
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
            size_t getHash(void) const;
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
            size_t getHash(void) const;
        };

        struct UnifiedBlendStateInformation
            : public BlendStateInformation
        {
            bool alphaToCoverage = false;

            void load(const JSON::Object &object);
            size_t getHash(void) const;
        };

        struct IndependentBlendStateInformation
        {
            bool alphaToCoverage = false;
            std::array<BlendStateInformation, 8> targetStates;

            void load(const JSON::Object &object);
            size_t getHash(void) const;
        };

        struct SamplerStateInformation
        {
            enum class FilterMode : uint8_t
            {
                MinificationMagnificationMipMapPoint = 0,
                MinificationMagnificationPointMipMapLinear,
                MinificationPointMagnificationLinearMipMapPoint,
                MinificationPointMagnificationMipMapLinear,
                MinificationLinearMagnificationMipMapPoint,
                MinificationLinearMagnificationPointMipMapLinear,
                MinificationMagnificationLinearMipMapPoint,
                MinificationMagnificationMipMapLinear,
                Anisotropic,
                ComparisonMinificationMagnificationMipMapPoint,
                ComparisonMinificationMagnificationPointMipMapLinear,
                ComparisonMinificationPointMagnificationLinearMipMapPoint,
                ComparisonMinificationPointMagnificationMipMapLinear,
                ComparisonMinificationLinearMagnificationMipMapPoint,
                ComparisonMinificationLinearMagnificationPointMipMapLinear,
                ComparisonMinificationMagnificationLinearMipMapPoint,
                ComparisonMinificationMagnificationMipMapLinear,
                ComparisonAnisotropic,
                MinimumMinificationMagnificationMipMapPoint,
                MinimumMinificationMagnificationPointMipMapLinear,
                MinimumMinificationPointMagnificationLinearMipMapPoint,
                MinimumMinificationPointMagnificationMipMapLinear,
                MinimumMinificationLinearMagnificationMipMapPoint,
                MinimumMinificationLinearMagnificationPointMipMapLinear,
                MinimumMinificationMagnificationLinearMipMapPoint,
                MinimumMinificationMagnificationMipMapLinear,
                MinimumAnisotropic,
                MaximumMinificationMagnificationMipMapPoint,
                MaximumMinificationMagnificationPointMipMapLinear,
                MaximumMinificationPointMagnificationLinearMipMapPoint,
                MaximumMinificationPointMagnificationMipMapLinear,
                MaximumMinificationLinearMagnificationMipMapPoint,
                MaximumMinificationLinearMagnificationPointMipMapLinear,
                MaximumMinificationMagnificationLinearMipMapPoint,
                MaximumMinificationMagnificationMipMapLinear,
                MaximumAnisotropic,
            };

			enum class AddressMode : uint8_t
			{
				Clamp = 0,
				Wrap,
				Mirror,
				MirrorOnce,
				Border,
			};

			FilterMode filterMode = FilterMode::MinificationMagnificationMipMapPoint;
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
            size_t getHash(void) const;
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

            static const uint32_t AppendAligned = 0xFFFFFFFF;

            static Source getSource(String const &elementSource);
            static Semantic getSemantic(String const &semantic);

            Video::Format format = Format::Unknown;
            Semantic semantic = Semantic::TexCoord;
			Source source = Source::Vertex;
            uint32_t sourceIndex = 0;
            uint32_t alignedByteOffset = AppendAligned;
        };

        GEK_INTERFACE(Object)
        {
            virtual ~Object(void) = default;

            virtual void setName(wchar_t const * const name) = 0;
        };

        GEK_INTERFACE(Buffer)
            : virtual public Object
        {
            struct Description
            {
                struct Flags
                {
                    enum
                    {
                        Staging = 1 << 0,
                        Mappable = 1 << 1,
                        Resource = 1 << 2,
                        UnorderedAccess = 1 << 3,
                        Counter = 1 << 4,
                    };
                }; // Flags

                enum class Type : uint8_t
                {
                    Raw = 0,
                    Vertex,
                    Index,
                    Constant,
                    Structured,
                };

                Video::Format format = Video::Format::Unknown;
                uint32_t stride = 0;
                uint32_t count = 0;
                Type type = Type::Raw;
                uint32_t flags = 0;

                size_t getHash(void) const;
            };

            virtual ~Buffer(void) = default;
        
            virtual const Buffer::Description &getDescription(void) const = 0;
        };

        GEK_INTERFACE(Texture)
            : virtual public Object
        {
            struct Description
            {
                struct Flags
                {
                    enum
                    {
                        RenderTarget = 1 << 0,
                        DepthTarget = 1 << 1,
                        Resource = 1 << 2,
                        UnorderedAccess = 1 << 3,
                    };
                }; // Flags

                Video::Format format = Video::Format::Unknown;
                uint32_t width = 1;
                uint32_t height = 1;
                uint32_t depth = 1;
                uint32_t mipMapCount = 1;
                uint32_t sampleCount = 1;
                uint32_t sampleQuality = 0;
                uint32_t flags = 0;

                size_t getHash(void) const;
            };

            virtual ~Texture(void) = default;
        
            virtual const Texture::Description &getDescription(void) const = 0;
        };

        GEK_INTERFACE(Target)
            : virtual public Texture
        {
            virtual ~Target(void) = default;
            
            virtual const Video::ViewPort &getViewPort(void) const = 0;
        };

        GEK_INTERFACE(Device)
        {
            struct Description
            {
                String device;
                Format displayFormat = Format::R8G8B8A8_UNORM_SRGB;
                uint32_t sampleCount = 1;
                uint32_t sampleQuality = 0;
            };

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
                virtual void resolveSamples(Texture *destination, Texture *source) = 0;

                virtual void clearState(void) = 0;
                virtual void clearUnorderedAccess(Object *object, Math::Float4 const &value) = 0;
                virtual void clearUnorderedAccess(Object *object, Math::UInt4 const &value) = 0;
                virtual void clearRenderTarget(Target *renderTarget, Math::Float4 const &clearColor) = 0;
                virtual void clearDepthStencilTarget(Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

                virtual void clearRenderTargetList(uint32_t count, bool depthBuffer) = 0;
                virtual void clearIndexBuffer(void) = 0;
                virtual void clearVertexBufferList(uint32_t count, uint32_t firstSlot) = 0;

                virtual void setViewportList(const std::vector<Video::ViewPort> &viewPortList) = 0;
                virtual void setScissorList(const std::vector<Math::UInt4> &rectangleList) = 0;
                virtual void setRenderTargetList(const std::vector<Target *> &renderTargetList, Object *depthBuffer) = 0;

                virtual void setRenderState(Object *renderState) = 0;
                virtual void setDepthState(Object *depthState, uint32_t stencilReference) = 0;
                virtual void setBlendState(Object *blendState, Math::Float4 const &blendFactor, uint32_t sampleMask) = 0;

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

			virtual char const * const getSemanticMoniker(InputElement::Semantic semantic) = 0;

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

            virtual TexturePtr createTexture(const Texture::Description &description, const void *data = nullptr) = 0;
            virtual TexturePtr loadTexture(const FileSystem::Path &filePath, uint32_t flags) = 0;
            virtual Texture::Description loadTextureDescription(const FileSystem::Path &filePath) = 0;

            virtual BufferPtr createBuffer(const Buffer::Description &description, const void *staticData = nullptr) = 0;

            template <typename TYPE>
            bool mapBuffer(Buffer *buffer, TYPE *&data, Video::Map mapping = Video::Map::WriteDiscard)
            {
                return mapBuffer(buffer, (void *&)data, mapping);
            }

            virtual bool mapBuffer(Buffer *buffer, void *&data, Video::Map mapping = Video::Map::WriteDiscard) = 0;
            virtual void unmapBuffer(Buffer *buffer) = 0;

            virtual void updateResource(Object *buffer, const void *data) = 0;
            virtual void copyResource(Object *destination, Object *source) = 0;

			virtual ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize) = 0;

            virtual std::vector<uint8_t> compileProgram(PipelineType pipelineType, wchar_t const * const name, wchar_t const * const uncompiledProgram, wchar_t const * const entryFunction) = 0;
            virtual ObjectPtr createProgram(PipelineType pipelineType, const void *compiledData, uint32_t compiledSize) = 0;

            virtual void executeCommandList(Object *commandList) = 0;

            virtual void present(bool waitForVerticalSync) = 0;
        };

        namespace Debug
        {
            GEK_INTERFACE(Device)
                : public Video::Device
            {
                virtual ~Device(void) = default;

                virtual void *getDevice(void) = 0;
            };
        }; // namespace Debug
    }; // namespace Video
}; // namespace Gek
