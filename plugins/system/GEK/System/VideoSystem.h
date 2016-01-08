#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector4.h"
#include "GEK\System\OverlaySystem.h"
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <unordered_map>

namespace Gek
{
    namespace Video
    {
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
                Readable = 1 << 0,
                Writable = 1 << 1,
                Resource = 1 << 2,
                UnorderedAccess = 1 << 3,
            };
        }; // BufferFlags

        namespace TextureFlags
        {
            enum
            {
                Resource = 1 << 0,
                UnorderedAccess = 1 << 1,
                MipMaps = 1 << 2,
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
                : nearDepth(0.0f)
                , farDepth(1.0f)
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

        struct RenderStates
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

            RenderStates(void)
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

        struct DepthStates
        {
            struct StencilStates
            {
                StencilOperation failOperation;
                StencilOperation depthFailOperation;
                StencilOperation passOperation;
                ComparisonFunction comparisonFunction;

                StencilStates(void)
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
            StencilStates stencilFrontStates;
            StencilStates stencilBackStates;

            DepthStates(void)
                : enable(false)
                , writeMask(DepthWrite::All)
                , comparisonFunction(ComparisonFunction::Always)
                , stencilEnable(false)
                , stencilReadMask(0xFF)
                , stencilWriteMask(0xFF)
            {
            }
        };

        struct TargetBlendStates
        {
            bool enable;
            BlendSource colorSource;
            BlendSource colorDestination;
            BlendOperation colorOperation;
            BlendSource alphaSource;
            BlendSource alphaDestination;
            BlendOperation alphaOperation;
            UINT8 writeMask;

            TargetBlendStates(void)
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

        struct UnifiedBlendStates : public TargetBlendStates
        {
            bool alphaToCoverage;

            UnifiedBlendStates(void)
                : alphaToCoverage(false)
            {
            }
        };

        struct IndependentBlendStates
        {
            bool alphaToCoverage;
            TargetBlendStates targetStates[8];

            IndependentBlendStates(void)
                : alphaToCoverage(false)
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

        struct SamplerStates
        {
            FilterMode filterMode;
            AddressMode addressModeU;
            AddressMode addressModeV;
            AddressMode addressModeW;
            float mipLevelBias;
            UINT32 maximumAnisotropy;
            ComparisonFunction comparisonFunction;
            Math::Float4 borderColor;
            float minimumMipLevel;
            float maximumMipLevel;

            SamplerStates(void)
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
    }; // namespace Video

    DECLARE_INTERFACE_IID(VideoBuffer, "8210EC49-9452-4BA1-A1F2-B17C0416A6B4") : virtual public IUnknown
    {
        STDMETHOD_(Video::Format, getFormat)                (THIS) PURE;
        STDMETHOD_(UINT32, getStride)                       (THIS) PURE;
        STDMETHOD_(UINT32, getCount)                        (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(VideoTexture, "D7793714-FE83-49BA-A68F-149246413867") : virtual public IUnknown
    {
        STDMETHOD_(Video::Format, getFormat)                (THIS) PURE;
        STDMETHOD_(UINT32, getWidth)                        (THIS) PURE;
        STDMETHOD_(UINT32, getHeight)                       (THIS) PURE;
        STDMETHOD_(UINT32, getDepth)                        (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(VideoTarget, "1F22B821-6FBF-4747-B913-502A40AA17AF") : virtual public VideoTexture
    {
        STDMETHOD_(const Video::ViewPort &, getViewPort)    (THIS) PURE;
    };

    DECLARE_INTERFACE(VideoPipeline)
    {
        STDMETHOD_(void, setProgram)                        (THIS_ IUnknown *program) PURE;
        STDMETHOD_(void, setConstantBuffer)                 (THIS_ VideoBuffer *constantBuffer, UINT32 stage) PURE;
        STDMETHOD_(void, setSamplerStates)                  (THIS_ IUnknown *samplerStates, UINT32 stage) PURE;
        STDMETHOD_(void, setResource)                       (THIS_ IUnknown *resource, UINT32 stage) PURE;
        STDMETHOD_(void, setUnorderedAccess)                (THIS_ IUnknown *unorderedAccess, UINT32 stage) { };
    };

    DECLARE_INTERFACE_IID(VideoContext, "95262C77-0F56-4447-9337-5819E68B372E") : virtual public IUnknown
    {
        STDMETHOD_(VideoPipeline *, computePipeline)        (THIS) PURE;
        STDMETHOD_(VideoPipeline *, vertexPipeline)         (THIS) PURE;
        STDMETHOD_(VideoPipeline *, geometryPipeline)       (THIS) PURE;
        STDMETHOD_(VideoPipeline *, pixelPipeline)          (THIS) PURE;

        STDMETHOD_(void, generateMipMaps)                   (THIS_ VideoTexture *texture) PURE;

        STDMETHOD_(void, clearResources)                    (THIS) PURE;

        STDMETHOD_(void, setViewports)                      (THIS_ Video::ViewPort *viewPortList, UINT32 viewPortCount) PURE;
        STDMETHOD_(void, setScissorRect)                    (THIS_ Shape::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount) PURE;

        STDMETHOD_(void, clearRenderTarget)                 (THIS_ VideoTarget *renderTarget, const Math::Float4 &colorClear) PURE;
        STDMETHOD_(void, clearDepthStencilTarget)           (THIS_ IUnknown *depthBuffer, DWORD flags, float depthClear, UINT32 stencilClear) PURE;
        STDMETHOD_(void, setRenderTargets)                  (THIS_ VideoTarget **renderTargetList, UINT32 renderTargetCount, IUnknown *depthBuffer) PURE;

        STDMETHOD_(void, setRenderStates)                   (THIS_ IUnknown *renderStates) PURE;
        STDMETHOD_(void, setDepthStates)                    (THIS_ IUnknown *depthStates, UINT32 stencilReference) PURE;
        STDMETHOD_(void, setBlendStates)                    (THIS_ IUnknown *blendStates, const Math::Float4 &blendFactor, UINT32 sampleMask) PURE;

        STDMETHOD_(void, setVertexBuffer)                   (THIS_ UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset) PURE;
        STDMETHOD_(void, setIndexBuffer)                    (THIS_ VideoBuffer *indexBuffer, UINT32 offset) PURE;
        STDMETHOD_(void, setPrimitiveType)                  (THIS_ Video::PrimitiveType type) PURE;

        STDMETHOD_(void, drawPrimitive)                     (THIS_ UINT32 vertexCount, UINT32 firstVertex) PURE;
        STDMETHOD_(void, drawInstancedPrimitive)            (THIS_ UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex) PURE;

        STDMETHOD_(void, drawIndexedPrimitive)              (THIS_ UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) PURE;
        STDMETHOD_(void, drawInstancedIndexedPrimitive)     (THIS_ UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) PURE;

        STDMETHOD_(void, dispatch)                          (THIS_ UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ) PURE;

        STDMETHOD_(void, finishCommandList)                 (THIS_ IUnknown **returnObject) PURE;
    };

    DECLARE_INTERFACE_IID(VideoSystem, "CA9BBC81-83E9-4C26-9BED-5BF3B2D189D6") : virtual public IUnknown
    {
        STDMETHOD(initialize)                               (THIS_ HWND window, bool fullScreen, Video::Format depthBufferFormat = Video::Format::Unknown) PURE;
        STDMETHOD(setFullScreen)                            (THIS_ bool fullScreen) PURE;
        STDMETHOD(resize)                                   (THIS) PURE;

        STDMETHOD_(VideoContext *, getDefaultContext)       (THIS) PURE;
        STDMETHOD_(OverlaySystem *, getOverlay)             (THIS) PURE;

        STDMETHOD_(UINT32, getWidth)                        (THIS) PURE;
        STDMETHOD_(UINT32, getHeight)                       (THIS) PURE;
        STDMETHOD_(bool, isFullScreen)                      (THIS) PURE;

        STDMETHOD(createDeferredContext)                    (THIS_ VideoContext **returnObject) PURE;

        STDMETHOD(createEvent)                              (THIS_ IUnknown **returnObject) PURE;
        STDMETHOD_(void, setEvent)                          (THIS_ IUnknown *event) PURE;
        STDMETHOD_(bool, isEventSet)                        (THIS_ IUnknown *event) PURE;

        STDMETHOD(createRenderStates)                       (THIS_ IUnknown **returnObject, const Video::RenderStates &renderStates) PURE;
        STDMETHOD(createDepthStates)                        (THIS_ IUnknown **returnObject, const Video::DepthStates &depthStates) PURE;
        STDMETHOD(createBlendStates)                        (THIS_ IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD(createBlendStates)                        (THIS_ IUnknown **returnObject, const Video::IndependentBlendStates &blendStates) PURE;
        STDMETHOD(createSamplerStates)                      (THIS_ IUnknown **returnObject, const Video::SamplerStates &samplerStates) PURE;

        STDMETHOD(createRenderTarget)                       (THIS_ VideoTarget **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD(createDepthTarget)                        (THIS_ IUnknown **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;

        STDMETHOD(createBuffer)                             (THIS_ VideoBuffer **returnObject, UINT32 stride, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD(createBuffer)                             (THIS_ VideoBuffer **returnObject, Video::Format format, UINT32 count, Video::BufferType type, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD_(void, updateBuffer)                      (THIS_ VideoBuffer *buffer, LPCVOID data) PURE;
        STDMETHOD(mapBuffer)                                (THIS_ VideoBuffer *buffer, LPVOID *data, Video::Map mapping = Video::Map::WriteDiscard) PURE;
        STDMETHOD_(void, unmapBuffer)                       (THIS_ VideoBuffer *buffer) PURE;
        STDMETHOD_(void, copyBuffer)                        (THIS_ VideoBuffer *destination, VideoBuffer *source) PURE;

        STDMETHOD(compileComputeProgram)                    (THIS_ IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(compileVertexProgram)                     (THIS_ IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(compileGeometryProgram)                   (THIS_ IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(compilePixelProgram)                      (THIS_ IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD(loadComputeProgram)                       (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(loadVertexProgram)                        (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(loadGeometryProgram)                      (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(loadPixelProgram)                         (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD(createTexture)                            (THIS_ VideoTexture **returnObject, UINT32 width, UINT32 height, UINT32 depth, Video::Format format, DWORD flags) PURE;
        STDMETHOD(loadTexture)                              (THIS_ VideoTexture **returnObject, LPCWSTR fileName, DWORD flags) PURE;
        STDMETHOD(loadCubeMap)                              (THIS_ VideoTexture **returnObject, LPCWSTR fileNameList[6], DWORD flags) PURE;
        STDMETHOD_(void, updateTexture)                     (THIS_ VideoTexture *texture, LPCVOID data, UINT32 pitch, Shape::Rectangle<UINT32> *rectangle = nullptr) PURE;

        STDMETHOD_(void, clearDefaultRenderTarget)          (THIS_ const Math::Float4 &colorClear) PURE;
        STDMETHOD_(void, clearDefaultDepthStencilTarget)    (THIS_ DWORD flags, float depthClear, UINT32 stencilClear) PURE;
        STDMETHOD_(void, setDefaultTargets)                 (THIS_ VideoContext *context = nullptr, IUnknown *depthBuffer = nullptr) PURE;

        STDMETHOD_(void, executeCommandList)                (THIS_ IUnknown *commandList) PURE;

        STDMETHOD_(void, present)                           (THIS_ bool waitForVerticalSync) PURE;
    };

    DECLARE_INTERFACE_IID(VideoSystemRegistration, "6B94910F-0D37-487E-92C3-B7C391108B44");
}; // namespace Gek
