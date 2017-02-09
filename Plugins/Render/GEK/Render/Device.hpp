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
    namespace Render
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
            Format format = Format::Unknown;
            AspectRatio aspectRatio = AspectRatio::Unknown;
            struct
            {
                uint32_t numerator = 0;
                uint32_t denominator = 0;
            } refreshRate;
        };

        using DisplayModeList = std::vector<DisplayMode>;

        namespace PipelineType
        {
            enum
            {
                Compute = 1 << 0,
                Vertex = 1 << 1,
                Geometry = 1 << 2,
                Pixel = 1 << 3,
            };
        }; // namespace PipelineType

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

        namespace RenderQueueFlags
        {
            enum
            {
                Direct = 1 << 0,
            };
        }; // namespace RenderQueueFlags

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

        struct RasterizerStateInformation
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

            struct TargetStateInformation
            {
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

            bool alphaToCoverage = false;
            bool unifiedBlendState = true;
            std::array<TargetStateInformation, 8> targetStateList;

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

            static const uint32_t AlignedByteOffset = 0xFFFFFFFF;

            static Source getSource(String const &elementSource);
            static Semantic getSemantic(String const &semantic);

            Format format = Format::Unknown;
            Semantic semantic = Semantic::TexCoord;
			Source source = Source::Vertex;
            uint32_t sourceIndex = 0;
            uint32_t alignedByteOffset = AlignedByteOffset;

            size_t getHash(void) const;
        };

        struct PipelineStateInformation
        {
            std::vector<uint8_t> compiledVertexShader;
            std::vector<uint8_t> compiledPixelShader;
            BlendStateInformation blendStateInformation;
            uint32_t sampleMask = 0xFFFFFFFF;
            RasterizerStateInformation rasterizerStateInformation;
            DepthStateInformation depthStateInformation;
            std::vector<InputElement> inputElementList;
            PrimitiveType primitiveType = PrimitiveType::TriangleList;
            uint32_t renderTargetCount = 1;
            Format renderTargetFormatList[8] = { Format::Unknown };
            Format depthTargetFormat = Format::Unknown;

            size_t getHash(void) const;
        };

        struct BufferDescription
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
                IndirectArguments,
            };

            Format format = Format::Unknown;
            uint32_t stride = 0;
            uint32_t count = 0;
            Type type = Type::Raw;
            uint32_t flags = 0;

            size_t getHash(void) const;
        };

        struct TextureDescription
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

            Format format = Format::Unknown;
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t depth = 1;
            uint32_t mipMapCount = 1;
            uint32_t sampleCount = 1;
            uint32_t sampleQuality = 0;
            uint32_t flags = 0;

            size_t getHash(void) const;
        };

        template <typename TYPE, uint8_t UNIQUE>
        struct Handle
        {
            TYPE identifier;

            Handle(uint32_t identifier = 0)
                : identifier(TYPE(identifier))
            {
            }

            operator bool() const
            {
                return (identifier != 0);
            }

            operator std::size_t() const
            {
                return identifier.load();
            }

            bool operator == (typename Handle<TYPE, UNIQUE> const &handle) const
            {
                return (identifier == handle.identifier);
            }

            bool operator != (typename Handle<TYPE, UNIQUE> const &handle) const
            {
                return (identifier != handle.identifier);
            }
        };

        using PipelineStateHandle = Handle<uint16_t, __LINE__>;
        using SamplerStateHandle = Handle<uint8_t, __LINE__>;
        using ProgramHandle = Handle<uint16_t, __LINE__>;
        using RenderListHandle = Handle<uint16_t, __LINE__>;
        using ResourceHandle = Handle<uint32_t, __LINE__>;

        GEK_INTERFACE(Device)
        {
            struct Description
            {
                String device;
                Format displayFormat = Format::R8G8B8A8_UNORM_SRGB;
                uint32_t sampleCount = 1;
                uint32_t sampleQuality = 0;
            };

            GEK_INTERFACE(RenderQueue)
            {
                struct Pipeline
                {
                    enum
                    {
                        Vertex = 1 << 0,
                        Pixel = 1 << 1,
                    };
                }; // Pipeline

                virtual ~RenderQueue(void) = default;

                virtual void generateMipMaps(ResourceHandle texture) = 0;
                virtual void resolveSamples(ResourceHandle destination, ResourceHandle source) = 0;

                virtual void clearState(void) = 0;

                virtual void clearUnorderedAccess(ResourceHandle object, Math::Float4 const &value) = 0;
                virtual void clearUnorderedAccess(ResourceHandle object, Math::UInt4 const &value) = 0;
                virtual void clearRenderTarget(ResourceHandle renderTarget, Math::Float4 const &clearColor) = 0;
                virtual void clearDepthStencilTarget(ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

                virtual void setViewportList(const std::vector<ViewPort> &viewPortList) = 0;
                virtual void setScissorList(const std::vector<Math::UInt4> &rectangleList) = 0;

                virtual void bindPipelineState(PipelineStateHandle pipelineState) = 0;

                virtual void bindSamplerStateList(const std::vector<SamplerStateHandle> &samplerStateList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
                virtual void bindConstantBufferList(const std::vector<ResourceHandle> &constantBufferList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
                virtual void bindResourceList(const std::vector<ResourceHandle> &resourceList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
                virtual void bindUnorderedAccessList(const std::vector<ResourceHandle> &unorderedAccessList, uint32_t firstStage, uint32_t *countList = nullptr) = 0;

                virtual void bindIndexBuffer(ResourceHandle indexBuffer, uint32_t offset) = 0;
                virtual void bindVertexBufferList(const std::vector<ResourceHandle> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList = nullptr) = 0;
                virtual void bindRenderTargetList(const std::vector<ResourceHandle> &renderTargetList, ResourceHandle depthBuffer) = 0;

                virtual void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex) = 0;
                virtual void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;
                virtual void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
                virtual void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
                virtual void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;

                virtual void drawInstancedPrimitive(ResourceHandle bufferArguments) = 0;
                virtual void drawInstancedIndexedPrimitive(ResourceHandle bufferArguments) = 0;
                virtual void dispatch(ResourceHandle bufferArguments) = 0;
            };

            virtual ~Device(void) = default;

            virtual DisplayModeList getDisplayModeList(Format format) const = 0;

            virtual void setFullScreenState(bool fullScreen) = 0;
            virtual void setDisplayMode(const DisplayMode &displayMode) = 0;
            virtual void handleResize(void) = 0;

			virtual char const * const getSemanticMoniker(InputElement::Semantic semantic) = 0;

            virtual PipelineStateHandle createPipelineState(const PipelineStateInformation &pipelineState) = 0;

            virtual SamplerStateHandle createSamplerState(const SamplerStateInformation &samplerState) = 0;

            virtual std::vector<uint8_t> compileProgram(uint32_t pipeline, wchar_t const * const name, wchar_t const * const uncompiledProgram, wchar_t const * const entryFunction) = 0;

            virtual ResourceHandle createBuffer(const BufferDescription &description, const void *staticData = nullptr) = 0;
            virtual ResourceHandle createTexture(const TextureDescription &description, const void *data = nullptr) = 0;
            virtual ResourceHandle loadTexture(const FileSystem::Path &filePath, uint32_t flags) = 0;

            template <typename TYPE>
            bool mapResource(ResourceHandle buffer, TYPE *&data, Map mapping = Map::WriteDiscard)
            {
                return mapResource(buffer, (void *&)data, mapping);
            }

            virtual bool mapResource(ResourceHandle resource, void *&data, Map mapping = Map::WriteDiscard) = 0;
            virtual void unmapResource(ResourceHandle resource) = 0;

            virtual void updateResource(ResourceHandle resource, const void *data) = 0;
            virtual void copyResource(ResourceHandle destination, ResourceHandle source) = 0;

            virtual RenderQueuePtr createRenderQueue(uint32_t flags) = 0;
            virtual RenderListHandle createRenderList(RenderQueue *renderQueue) = 0;
            virtual void executeRenderList(RenderListHandle renderList) = 0;

            virtual void present(bool waitForVerticalSync) = 0;
        };

        namespace Debug
        {
            GEK_INTERFACE(Device)
                : public Render::Device
            {
                virtual ~Device(void) = default;

                virtual void *getDevice(void) = 0;
            };
        }; // namespace Debug
    }; // namespace Render
}; // namespace Gek

namespace std
{
    template <typename TYPE, int UNIQUE>
    struct hash<Gek::Render::Handle<TYPE, UNIQUE>>
    {
        size_t operator()(const Gek::Render::Handle<TYPE, UNIQUE> &value) const
        {
            return value.identifier;
        }
    };
};
