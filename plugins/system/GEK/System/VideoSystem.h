#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Float2.h"
#include "GEK\Math\Float4.h"
#include "GEK\Shapes\Rectangle.h"
#include "GEK\Utility\Hash.h"
#include "GEK\Utility\Trace.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <memory>
#include <unordered_map>

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

        struct UnifiedBlendState : public TargetBlendState
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
            LPCSTR semanticName;
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

            InputElement(Format format, LPCSTR semanticName, UINT32 semanticIndex, ElementType slotClass = ElementType::Vertex, UINT32 slotIndex = 0)
                : format(format)
                , semanticName(semanticName)
                , semanticIndex(semanticIndex)
                , slotClass(slotClass)
                , slotIndex(slotIndex)
            {
            }
        };
    }; // namespace Video

    interface VideoObject
    {
    };

    interface VideoBuffer : public VideoObject
    {
        Video::Format getFormat(void);
        UINT32 getStride(void);
        UINT32 getCount(void);
    };

    interface VideoTexture : public VideoObject
    {
        Video::Format getFormat(void);
        UINT32 getWidth(void);
        UINT32 getHeight(void);
        UINT32 getDepth(void);
    };

    interface VideoTarget : public VideoTexture
    {
        const Video::ViewPort &getViewPort(void);
    };

    interface VideoPipeline
    {
        void setProgram(VideoObject *program);
        void setConstantBuffer(VideoBuffer *constantBuffer, UINT32 stage);
        void setSamplerState(VideoObject *samplerState, UINT32 stage);
        void setResource(VideoObject *resource, UINT32 stage);
        void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage) = default;
    };

    interface VideoContext
    {
        VideoPipeline *computePipeline(void);
        VideoPipeline *vertexPipeline(void);
        VideoPipeline *geometryPipeline(void);
        VideoPipeline *pixelPipeline(void);

        void generateMipMaps(VideoTexture *texture);

        void clearResources(void);

        void setViewports(Video::ViewPort *viewPortList, UINT32 viewPortCount);
        void setScissorRect(Shapes::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount);

        void clearRenderTarget(VideoTarget *renderTarget, const Math::Color &colorClear);
        void clearDepthStencilTarget(VideoObject *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear);
        void clearUnorderedAccessBuffer(VideoObject *buffer, float value);
        void setRenderTargets(VideoTarget **renderTargetList, UINT32 renderTargetCount, VideoObject *depthBuffer);

        void setRenderState(VideoObject *renderState);
        void setDepthState(VideoObject *depthState, UINT32 stencilReference);
        void setBlendState(VideoObject *blendState, const Math::Color &blendFactor, UINT32 sampleMask);

        void setVertexBuffer(UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset);
        void setIndexBuffer(VideoBuffer *indexBuffer, UINT32 offset);
        void setPrimitiveType(Video::PrimitiveType type);

        void drawPrimitive(UINT32 vertexCount, UINT32 firstVertex);
        void drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex);

        void drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex);
        void drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex);

        void dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ);

        std::shared_ptr<VideoObject> finishCommandList(void);
    };

    interface VideoSystem
    {
        void initialize(HWND window, bool fullScreen, Video::Format format);
        void setFullScreen(bool fullScreen);
        void setSize(UINT32 width, UINT32 height, Video::Format format);
        void resize(void);

        VideoTarget * const getBackBuffer(void);
        VideoContext * const getDefaultContext(void);

        bool isFullScreen(void);

        std::shared_ptr<VideoContext> createDeferredContext(void);

        std::shared_ptr<VideoObject> createEvent(void);
        void setEvent(VideoObject *event);
        bool isEventSet(VideoObject *event);

        std::shared_ptr<VideoObject> createRenderState(const Video::RenderState &renderState);
        std::shared_ptr<VideoObject> createDepthState(const Video::DepthState &depthState);
        std::shared_ptr<VideoObject> createBlendState(const Video::UnifiedBlendState &blendState);
        std::shared_ptr<VideoObject> createBlendState(const Video::IndependentBlendState &blendState);
        std::shared_ptr<VideoObject> createSamplerState(const Video::SamplerState &samplerState);

        std::shared_ptr<VideoTexture> createTexture(Video::Format format, UINT32 width, UINT32 height, UINT32 depth, UINT32 flags, UINT32 mipmaps = 1);
        std::shared_ptr<VideoTexture> loadTexture(LPCWSTR fileName, UINT32 flags);
        std::shared_ptr<VideoTexture> loadCubeMap(LPCWSTR fileNameList[6], UINT32 flags);
        void updateTexture(VideoTexture *texture, LPCVOID data, UINT32 pitch, Shapes::Rectangle<UINT32> *rectangle = nullptr);

        std::shared_ptr<VideoBuffer> createBuffer(UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID staticData = nullptr);
        std::shared_ptr<VideoBuffer> createBuffer(Video::Format format, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID staticData = nullptr);
        void updateBuffer(VideoBuffer *buffer, LPCVOID data);
        void mapBuffer(VideoBuffer *buffer, void **data, Video::Map mapping = Video::Map::WriteDiscard);
        void unmapBuffer(VideoBuffer *buffer);

        void copyResource(VideoObject *destination, VideoObject *source);

        std::shared_ptr<VideoObject> compileComputeProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> compileVertexProgram(LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout = std::vector<Video::InputElement>(), std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> compileGeometryProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> compilePixelProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());

        std::shared_ptr<VideoObject> loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> loadVertexProgram(LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout = std::vector<Video::InputElement>(), std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> loadGeometryProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());
        std::shared_ptr<VideoObject> loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, const std::unordered_map<CStringA, CStringA> &defineList = std::unordered_map<CStringA, CStringA>());

        void executeCommandList(VideoObject *commandList);

        void present(bool waitForVerticalSync);
    };

    DECLARE_INTERFACE_IID(VideoSystemRegistration, "6B94910F-0D37-487E-92C3-B7C391108B44");
}; // namespace Gek
