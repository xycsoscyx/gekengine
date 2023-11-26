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
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/Context.hpp"
#include <unordered_map>
#include <functional>
#include <memory>
#include <array>

#ifdef _WINDLL
#   define DLL_API __declspec(dllexport)
#else
#   define DLL_API __declspec(dllimport)
#endif

namespace Gek
{
    namespace Render
    {
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

        Format GetFormat(std::string const& format);

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

        namespace Pipeline
        {
            enum
            {
                Compute = 1 << 0,
                Vertex = 1 << 1,
                Geometry = 1 << 2,
                Pixel = 1 << 3,
            };
        }; // namespace Pipeline

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

            ViewPort(Math::Float2 const& position, Math::Float2 const& size, float nearClip, float farClip)
                : position(position)
                , size(size)
                , nearClip(nearClip)
                , farClip(farClip)
            {
            }
        };

        GEK_INTERFACE(Resource)
        {
            virtual ~Resource(void) = default;

            virtual std::string_view getName(void) const = 0;
        };

        GEK_INTERFACE(PipelineFormat)
            : virtual public Resource
        {
            struct Descriptor
            {
                uint32_t index;
                uint32_t space;
            };

            struct DescriptorRange
                : public Descriptor
            {
                enum class Type : uint8_t
                {
                    ResourceView = 0,
                    UnorderedView,
                    ConstantView,
                    Sampler,
                };

                Type type;
                uint32_t count;
                uint32_t offset;
            };

            struct DescriptorTable
            {
                std::vector<DescriptorRange> descriptorRanges;
            };

            struct Parameter
            {
                enum class Type : uint8_t
                {
                    DescriptorTable = 0,
                    ResourceView,
                    UnorderedView,
                    ConstantView,
                };

                enum class Visibility : uint8_t
                {
                    All = 0,
                    Vertex,
                    Pixel,
                };

                Type type;
                Visibility visibility;
                union
                {
                    Descriptor descriptor;
                    DescriptorRange DescriptorTable;
                };
            };

            struct Sampler
            {
            };

            struct Description
            {
                std::string name;
                std::vector<Parameter> parameters;
                std::vector<Sampler> samplers;
            };

            virtual ~PipelineFormat(void) = default;

            virtual Description const& getDescription(void) const = 0;
        };

        GEK_INTERFACE(PipelineState)
            : virtual public Resource
        {
            struct RasterizerState
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

                struct Description
                {
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

                    void load(JSON::Object const& object);
                };
            };

            struct DepthState
            {
                enum class Write : uint8_t
                {
                    Zero = 0,
                    All,
                };

                struct StencilState
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

                    struct Description
                    {
                        Operation failOperation = Operation::Keep;
                        Operation depthFailOperation = Operation::Keep;
                        Operation passOperation = Operation::Keep;
                        ComparisonFunction comparisonFunction = ComparisonFunction::Always;

                        void load(JSON::Object const& object);
                    };
                };

                struct Description
                {
                    bool enable = false;
                    Write writeMask = Write::All;
                    ComparisonFunction comparisonFunction = ComparisonFunction::Always;
                    bool stencilEnable = false;
                    uint8_t stencilReadMask = 0xFF;
                    uint8_t stencilWriteMask = 0xFF;
                    StencilState::Description stencilFrontState;
                    StencilState::Description stencilBackState;

                    void load(JSON::Object const& object);
                };
            };

            struct BlendState
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

                struct TargetState
                {
                    struct Description
                    {
                        bool enable = false;
                        Source colorSource = Source::One;
                        Source colorDestination = Source::One;
                        Operation colorOperation = Operation::Add;
                        Source alphaSource = Source::One;
                        Source alphaDestination = Source::One;
                        Operation alphaOperation = Operation::Add;
                        uint8_t writeMask = Mask::RGBA;

                        void load(JSON::Object const& object);
                    };
                };

                struct Description
                {
                    bool alphaToCoverage = false;
                    bool unifiedBlendState = true;
                    std::array<TargetState::Description, 8> targetStateList;

                    void load(JSON::Object const& object);
                };
            };

            struct RenderTarget
            {
                std::string name;
                Format format = Format::Unknown;
            };

            struct ElementDeclaration
            {
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

                static Semantic GetSemantic(std::string const& semantic);

                std::string name;
                Format format = Format::Unknown;
                Semantic semantic = Semantic::TexCoord;
            };

            struct VertexDeclaration
                : public ElementDeclaration
            {
                enum class Source : uint8_t
                {
                    Vertex = 0,
                    Instance,
                };

                static constexpr uint32_t AppendAligned = 0xFFFFFFFF;

                static Source GetSource(std::string const& elementSource);

                Source source = Source::Vertex;
                uint32_t sourceIndex = 0;
                uint32_t alignedByteOffset = AppendAligned;
            };

            struct Description
            {
                std::string name;

                BlendState::Description blendStateDescription;
                uint32_t sampleMask = 0xFFFFFFFF;

                RasterizerState::Description rasterizerStateDescription;

                DepthState::Description depthStateDescription;

                PrimitiveType primitiveType = PrimitiveType::TriangleList;

                std::vector<VertexDeclaration> vertexDeclaration;
                std::vector<ElementDeclaration> pixelDeclaration;
                std::string vertexShader, vertexShaderEntryFunction;
                std::string pixelShader, pixelShaderEntryFunction;

                std::vector<RenderTarget> renderTargetList;
                Format depthTargetFormat = Format::Unknown;
            };

            virtual ~PipelineState(void) = default;

            virtual Description const& getDescription(void) const = 0;
        };

        GEK_INTERFACE(SamplerState)
            : virtual public Resource
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

            struct Description
            {
                std::string name;
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

                void load(JSON::Object const& object);
            };

            virtual ~SamplerState(void) = default;

            virtual Description const& getDescription(void) const = 0;
        };


        GEK_INTERFACE(Buffer)
            : virtual public Resource
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

            struct Description
            {
                std::string name;
                Format format = Format::Unknown;
                uint32_t stride = 0;
                uint32_t count = 0;
                Type type = Type::Raw;
                uint32_t flags = 0;
            };

            virtual ~Buffer(void) = default;

            virtual Description const& getDescription(void) const = 0;
        };

        GEK_INTERFACE(Texture)
            : virtual public Resource
        {
            struct Flags
            {
                enum
                {
                    RenderTarget = 1 << 0,
                    DepthTarget = 1 << 1,
                    ResourceView = 1 << 2,
                    UnorderedView = 1 << 3,
                };
            }; // Flags

            struct Description
            {
                std::string name;
                Format format = Format::Unknown;
                uint32_t width = 1;
                uint32_t height = 1;
                uint32_t depth = 1;
                uint32_t mipMapCount = 1;
                uint32_t sampleCount = 1;
                uint32_t sampleQuality = 0;
                uint32_t flags = 0;
            };

            virtual ~Texture(void) = default;

            virtual Description const& getDescription(void) const = 0;
        };

        GEK_INTERFACE(CommandList)
            : public Resource
        {
            struct Flags
            {
                enum
                {
                };
            }; // struct Flags

            virtual ~CommandList(void) = default;

            virtual void finish(void) = 0;

            virtual void generateMipMaps(Resource* texture) = 0;
            virtual void resolveSamples(Resource* destination, Resource* source) = 0;

            virtual void clearUnorderedAccess(Resource* object, Math::Float4 const& value) = 0;
            virtual void clearUnorderedAccess(Resource* object, Math::UInt4 const& value) = 0;
            virtual void clearRenderTarget(Texture* renderTarget, Math::Float4 const& clearColor) = 0;
            virtual void clearDepthStencilTarget(Texture* depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil) = 0;

            virtual void setViewportList(const std::vector<ViewPort>& viewPortList) = 0;
            virtual void setScissorList(const std::vector<Math::UInt4>& rectangleList) = 0;

            virtual void bindPipelineState(PipelineState* pipelineState) = 0;

            virtual void bindSamplerStateList(const std::vector<SamplerState*>& samplerStateList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
            virtual void bindConstantBufferList(const std::vector<Buffer*>& constantBufferList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
            virtual void bindResourceList(const std::vector<Resource*>& resourceList, uint32_t firstStage, uint8_t pipelineFlags) = 0;
            virtual void bindUnorderedAccessList(const std::vector<Resource*>& unorderedAccessList, uint32_t firstStage, uint32_t* countList = nullptr) = 0;

            virtual void bindIndexBuffer(Resource* indexBuffer, uint32_t offset) = 0;
            virtual void bindVertexBufferList(const std::vector<Resource*>& vertexBufferList, uint32_t firstSlot, uint32_t* offsetList = nullptr) = 0;
            virtual void bindRenderTargetList(const std::vector<Texture*>& renderTargetList, Texture* depthBuffer) = 0;

            virtual void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex) = 0;
            virtual void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex) = 0;
            virtual void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ) = 0;

            virtual void drawInstancedPrimitive(Resource* bufferArguments) = 0;
            virtual void drawInstancedIndexedPrimitive(Resource* bufferArguments) = 0;
            virtual void dispatch(Resource* bufferArguments) = 0;
        };

        GEK_INTERFACE(Queue)
            : public Resource
        {
            enum class Type : uint8_t
            {
                Direct = 0,
                Compute,
                Copy,
            };

            virtual ~Queue(void) = default;

            virtual void executeCommandList(CommandList* commandList) = 0;
        };

        GEK_INTERFACE(Device)
        {
            struct Description
            {
                std::string name;
                std::string device;
                Format displayFormat = Format::R8G8B8A8_UNORM_SRGB;
                uint32_t sampleCount = 1;
                uint32_t sampleQuality = 0;
            };

            virtual ~Device(void) = default;

            virtual DisplayModeList getDisplayModeList(Format format) const = 0;

            virtual Texture* getBackBuffer(void) = 0;
            virtual void setFullScreenState(bool fullScreen) = 0;
            virtual void setDisplayMode(const DisplayMode & displayMode) = 0;
            virtual void handleResize(void) = 0;

            virtual PipelineFormatPtr createPipelineFormat(const PipelineFormat::Description & pipelineDescription) = 0;
            virtual PipelineStatePtr createPipelineState(PipelineFormat *pipelineFormat, const PipelineState::Description & pipelineDescription) = 0;
            virtual SamplerStatePtr createSamplerState(PipelineFormat* pipelineFormat, const SamplerState::Description & samplerDescription) = 0;

            virtual BufferPtr createBuffer(const Buffer::Description & description, const void* staticData = nullptr) = 0;
            virtual TexturePtr createTexture(const Texture::Description & description, const void* data = nullptr) = 0;
            virtual TexturePtr loadTexture(FileSystem::Path const& filePath, uint32_t flags) = 0;

            template <typename TYPE>
            bool mapResource(Resource * buffer, TYPE * &data, Map mapping = Map::WriteDiscard)
            {
                return mapResource(buffer, (void*&)data, mapping);
            }

            virtual bool mapResource(Resource * resource, void*& data, Map mapping = Map::WriteDiscard) = 0;
            virtual void unmapResource(Resource * resource) = 0;

            virtual void updateResource(Resource * resource, const void* data) = 0;
            virtual void copyResource(Resource * destination, Resource * source) = 0;

            virtual QueuePtr createQueue(Queue::Type type) = 0;
            virtual CommandListPtr createCommandList(uint32_t flags) = 0;

            virtual void present(bool waitForVerticalSync) = 0;
        };

        namespace Debug
        {
            GEK_INTERFACE(Device)
                : public Render::Device
            {
                virtual ~Device(void) = default;

                virtual void* getDevice(void) = 0;
            };
        }; // namespace Debug
    }; // namespace Render
}; // namespace Gek