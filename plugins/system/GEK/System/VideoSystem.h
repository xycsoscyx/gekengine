#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Float2.h"
#include "GEK\Math\Float4.h"
#include "GEK\Shapes\Rectangle.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Context\Context.h"
#include <unordered_map>
#include <functional>
#include <memory>

namespace Gek
{
    namespace Video
    {
        GEK_BASE_EXCEPTION();

        enum class Format : UINT8
        {
            Unknown = 0,
            Byte,
            Byte2,
            Byte3,
            Byte4,
            BGRA,
            sRGBA,
            Short,
            Short2,
            Short4,
            Int,
            Int2,
            Int3,
            Int4,
            Half,
            Half2,
            Half3,
            Half4,
            Float,
            Float2,
            Float3,
            Float4,
            Depth16,
            Depth24Stencil8,
            Depth32,
            NumFormats
        };

        enum class ElementType : UINT8
        {
            Vertex = 0,
            Instance,
        };

        enum class FillMode : UINT8
        {
            WireFrame = 0,
            Solid,
        };

        enum class CullMode : UINT8
        {
            None = 0,
            Front,
            Back,
        };

        enum class DepthWrite : UINT8
        {
            Zero = 0,
            All,
        };

        enum class ComparisonFunction : UINT8
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

        enum class StencilOperation : UINT8
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

        enum class BlendSource : UINT8
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

        enum class BlendOperation : UINT8
        {
            Add = 0,
            Subtract,
            ReverseSubtract,
            Minimum,
            Maximum,
        };

        namespace ColorMask
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
        }; // namespace ColorMask

        enum class PrimitiveType : UINT8
        {
            PointList = 0,
            LineList,
            LineStrip,
            TriangleList,
            TriangleStrip,
        };

        enum class FilterMode : UINT8
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

        enum class AddressMode : UINT8
        {
            Clamp = 0,
            Wrap,
            Mirror,
            MirrorOnce,
            Border,
        };

        enum class Map : UINT8
        {
            Read = 0,
            Write,
            ReadWrite,
            WriteDiscard,
            WriteNoOverwrite,
        };

        enum class BufferType : UINT8
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

        namespace ClearMask
        {
            enum
            {
                Depth = 1 << 0,
                Stencil = 1 << 1,
            };
        }; // ClearMask

        struct ViewPort
        {
            Math::Float2 position;
            Math::Float2 size;
            float nearDepth;
            float farDepth;

            ViewPort(void)
            {
            }

            ViewPort(const Math::Float2 &position, const Math::Float2 &size, float nearDepth, float farDepth)
                : position(position)
                , size(size)
                , nearDepth(nearDepth)
                , farDepth(farDepth)
            {
            }
        };

        struct RenderState
        {
            FillMode fillMode;
            CullMode cullMode;
            bool frontCounterClockwise;
            UINT32 depthBias;
            float depthBiasClamp;
            float slopeScaledDepthBias;
            bool depthClipEnable;
            bool scissorEnable;
            bool multisampleEnable;
            bool antialiasedLineEnable;

            RenderState(void)
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

        struct DepthState
        {
            struct StencilState
            {
                StencilOperation failOperation;
                StencilOperation depthFailOperation;
                StencilOperation passOperation;
                ComparisonFunction comparisonFunction;

                StencilState(void)
                    : failOperation(StencilOperation::Keep)
                    , depthFailOperation(StencilOperation::Keep)
                    , passOperation(StencilOperation::Keep)
                    , comparisonFunction(ComparisonFunction::Always)
                {
                }
            };

            bool enable;
            DepthWrite writeMask;
            ComparisonFunction comparisonFunction;
            bool stencilEnable;
            UINT8 stencilReadMask;
            UINT8 stencilWriteMask;
            StencilState stencilFrontState;
            StencilState stencilBackState;

            DepthState(void)
                : enable(false)
                , writeMask(DepthWrite::All)
                , comparisonFunction(ComparisonFunction::Always)
                , stencilEnable(false)
                , stencilReadMask(0xFF)
                , stencilWriteMask(0xFF)
            {
            }
        };

        struct TargetBlendState
        {
            bool enable;
            BlendSource colorSource;
            BlendSource colorDestination;
            BlendOperation colorOperation;
            BlendSource alphaSource;
            BlendSource alphaDestination;
            BlendOperation alphaOperation;
            UINT8 writeMask;

            TargetBlendState(void)
                : enable(false)
                , colorSource(BlendSource::One)
                , colorDestination(BlendSource::One)
                , colorOperation(BlendOperation::Add)
                , alphaSource(BlendSource::One)
                , alphaDestination(BlendSource::One)
                , alphaOperation(BlendOperation::Add)
                , writeMask(ColorMask::RGBA)
            {
            }
        };

        struct UnifiedBlendState
            : public TargetBlendState
        {
            bool alphaToCoverage;

            UnifiedBlendState(void)
                : alphaToCoverage(false)
            {
            }
        };

        struct IndependentBlendState
        {
            bool alphaToCoverage;
            TargetBlendState targetStates[8];

            IndependentBlendState(void)
                : alphaToCoverage(false)
            {
            }
        };

        struct SamplerState
        {
            FilterMode filterMode;
            AddressMode addressModeU;
            AddressMode addressModeV;
            AddressMode addressModeW;
            float mipLevelBias;
            UINT32 maximumAnisotropy;
            ComparisonFunction comparisonFunction;
            Math::Color borderColor;
            float minimumMipLevel;
            float maximumMipLevel;

            SamplerState(void)
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
            Format format;
            const char *semanticName;
            UINT32 semanticIndex;
            ElementType slotClass;
            UINT32 slotIndex;

            InputElement(void)
                : format(Format::Unknown)
                , semanticName(nullptr)
                , semanticIndex(0)
                , slotClass(ElementType::Vertex)
                , slotIndex(0)
            {
            }

            InputElement(Format format, const char *semanticName, UINT32 semanticIndex, ElementType slotClass = ElementType::Vertex, UINT32 slotIndex = 0)
                : format(format)
                , semanticName(semanticName)
                , semanticIndex(semanticIndex)
                , slotClass(slotClass)
                , slotIndex(slotIndex)
            {
            }
        };
    }; // namespace Video

    GEK_INTERFACE(VideoObject)
    {
        virtual ~VideoObject(void) = default;
    };

    GEK_INTERFACE(VideoBuffer)
        : virtual public VideoObject
    {
        virtual Video::Format getFormat(void) = 0;
        virtual UINT32 getStride(void) = 0;
        virtual UINT32 getCount(void) = 0;
    };

    GEK_INTERFACE(VideoTexture)
        : virtual public VideoObject
    {
        virtual Video::Format getFormat(void) = 0;
        virtual UINT32 getWidth(void) = 0;
        virtual UINT32 getHeight(void) = 0;
        virtual UINT32 getDepth(void) = 0;
    };

    GEK_INTERFACE(VideoTarget)
        : virtual public VideoTexture
    {
        virtual const Video::ViewPort &getViewPort(void) = 0;
    };

    GEK_INTERFACE(VideoPipeline)
    {
        virtual void setProgram(VideoObject *program) = 0;
        virtual void setConstantBuffer(VideoBuffer *constantBuffer, UINT32 stage) = 0;
        virtual void setSamplerState(VideoObject *samplerState, UINT32 stage) = 0;
        virtual void setResource(VideoObject *resource, UINT32 stage) = 0;
        virtual void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage) = 0;
    };

    GEK_INTERFACE(VideoContext)
    {
        virtual VideoPipeline * const computePipeline(void) = 0;
        virtual VideoPipeline * const vertexPipeline(void) = 0;
        virtual VideoPipeline * const geometryPipeline(void) = 0;
        virtual VideoPipeline * const pixelPipeline(void) = 0;

        virtual void generateMipMaps(VideoTexture *texture) = 0;

        virtual void clearResources(void) = 0;

        virtual void setViewports(Video::ViewPort *viewPortList, UINT32 viewPortCount) = 0;
        virtual void setScissorRect(Shapes::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount) = 0;

        virtual void clearRenderTarget(VideoTarget *renderTarget, const Math::Color &colorClear) = 0;
        virtual void clearDepthStencilTarget(VideoObject *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear) = 0;
        virtual void setRenderTargets(VideoTarget **renderTargetList, UINT32 renderTargetCount, VideoObject *depthBuffer) = 0;

        virtual void setRenderState(VideoObject *renderState) = 0;
        virtual void setDepthState(VideoObject *depthState, UINT32 stencilReference) = 0;
        virtual void setBlendState(VideoObject *blendState, const Math::Color &blendFactor, UINT32 sampleMask) = 0;

        virtual void setVertexBuffer(UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset) = 0;
        virtual void setIndexBuffer(VideoBuffer *indexBuffer, UINT32 offset) = 0;
        virtual void setPrimitiveType(Video::PrimitiveType type) = 0;

        virtual void drawPrimitive(UINT32 vertexCount, UINT32 firstVertex) = 0;
        virtual void drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex) = 0;

        virtual void drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) = 0;
        virtual void drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) = 0;

        virtual void dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ) = 0;

        virtual VideoObjectPtr finishCommandList(void) = 0;
    };

    GEK_INTERFACE(VideoSystem)
    {
    public:
        virtual void setFullScreen(bool fullScreen) = 0;
        virtual void setSize(UINT32 width, UINT32 height, Video::Format format) = 0;
        virtual void resize(void) = 0;

        virtual VideoTarget * const getBackBuffer(void) = 0;
        virtual VideoContext * const getDefaultContext(void) = 0;

        virtual bool isFullScreen(void) = 0;

        virtual VideoContextPtr createDeferredContext(void) = 0;

        virtual VideoObjectPtr createEvent(void) = 0;
        virtual void setEvent(VideoObject *event) = 0;
        virtual bool isEventSet(VideoObject *event) = 0;

        virtual VideoObjectPtr createRenderState(const Video::RenderState &renderState) = 0;
        virtual VideoObjectPtr createDepthState(const Video::DepthState &depthState) = 0;
        virtual VideoObjectPtr createBlendState(const Video::UnifiedBlendState &blendState) = 0;
        virtual VideoObjectPtr createBlendState(const Video::IndependentBlendState &blendState) = 0;
        virtual VideoObjectPtr createSamplerState(const Video::SamplerState &samplerState) = 0;

        virtual VideoTexturePtr createTexture(Video::Format format, UINT32 width, UINT32 height, UINT32 depth, UINT32 flags, UINT32 mipmaps = 1) = 0;
        virtual VideoTexturePtr loadTexture(const wchar_t *fileName, UINT32 flags) = 0;
        virtual VideoTexturePtr loadCubeMap(const wchar_t *fileNameList[6], UINT32 flags) = 0;
        virtual void updateTexture(VideoTexture *texture, const void *data, UINT32 pitch, Shapes::Rectangle<UINT32> *rectangle = nullptr) = 0;

        virtual VideoBufferPtr createBuffer(UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, const void *staticData = nullptr) = 0;
        virtual VideoBufferPtr createBuffer(Video::Format format, UINT32 count, Video::BufferType type, UINT32 flags, const void *staticData = nullptr) = 0;
        virtual void updateBuffer(VideoBuffer *buffer, const void *data) = 0;
        virtual void mapBuffer(VideoBuffer *buffer, void **data, Video::Map mapping = Video::Map::WriteDiscard) = 0;
        virtual void unmapBuffer(VideoBuffer *buffer) = 0;

        virtual void copyResource(VideoObject *destination, VideoObject *source) = 0;

        virtual VideoObjectPtr compileComputeProgram(const char *programScript, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr compileVertexProgram(const char *programScript, const char *entryFunction, const std::vector<Video::InputElement> &elementLayout = std::vector<Video::InputElement>(), std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr compileGeometryProgram(const char *programScript, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr compilePixelProgram(const char *programScript, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;

        virtual VideoObjectPtr loadComputeProgram(const wchar_t *fileName, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr loadVertexProgram(const wchar_t *fileName, const char *entryFunction, const std::vector<Video::InputElement> &elementLayout = std::vector<Video::InputElement>(), std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr loadGeometryProgram(const wchar_t *fileName, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;
        virtual VideoObjectPtr loadPixelProgram(const wchar_t *fileName, const char *entryFunction, std::function<HRESULT(const char *, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<string, string> &defineList = std::unordered_map<string, string>()) = 0;

        virtual void executeCommandList(VideoObject *commandList) = 0;

        virtual void present(bool waitForVerticalSync) = 0;
    };
}; // namespace Gek
