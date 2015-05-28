#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Utility\Common.h"
#include <unordered_map>

namespace Gek
{
    namespace Video3D
    {
        static const Handle DefaultResourcePool = -1;

        enum class Format : UINT8
        {
            UNKNOWN = 0,
            R_UINT8,
            RG_UINT8,
            RGBA_UINT8,
            BGRA_UINT8,
            R_UINT16,
            RG_UINT16,
            RGBA_UINT16,
            R_UINT32,
            RG_UINT32,
            RGB_UINT32,
            RGBA_UINT32,
            R_FLOAT,
            RG_FLOAT,
            RGB_FLOAT,
            RGBA_FLOAT,
            R_HALF,
            RG_HALF,
            RGBA_HALF,
            D16,
            D24_S8,
            D32,
        };

        enum class ElementType : UINT8
        {
            VERTEX = 0,
            INSTANCE,
        };

        enum class FillMode : UINT8
        {
            WIREFRAME = 0,
            SOLID,
        };

        enum class CullMode : UINT8
        {
            NONE = 0,
            FRONT,
            BACK,
        };

        enum class DepthWrite : UINT8
        {
            ZERO = 0,
            ALL,
        };

        enum class ComparisonFunction : UINT8
        {
            ALWAYS = 0,
            NEVER,
            EQUAL,
            NOT_EQUAL,
            LESS,
            LESS_EQUAL,
            GREATER,
            GREATER_EQUAL,
        };

        enum class StencilOperation : UINT8
        {
            ZERO = 0,
            KEEP,
            REPLACE,
            INVERT,
            INCREASE,
            INCREASE_SATURATED,
            DECREASE,
            DECREASE_SATURATED,
        };

        enum class BlendSource : UINT8
        {
            ZERO = 0,
            ONE,
            BLENDFACTOR,
            INVERSE_BLENDFACTOR,
            SOURCE_COLOR,
            INVERSE_SOURCE_COLOR,
            SOURCE_ALPHA,
            INVERSE_SOURCE_ALPHA,
            SOURCE_ALPHA_SATURATE,
            DESTINATION_COLOR,
            INVERSE_DESTINATION_COLOR,
            DESTINATION_ALPHA,
            INVERSE_DESTINATION_ALPHA,
            SECONRARY_SOURCE_COLOR,
            INVERSE_SECONRARY_SOURCE_COLOR,
            SECONRARY_SOURCE_ALPHA,
            INVERSE_SECONRARY_SOURCE_ALPHA,
        };

        enum class BlendOperation : UINT8
        {
            ADD = 0,
            SUBTRACT,
            REVERSE_SUBTRACT,
            MINIMUM,
            MAXIMUM,
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
            POINTLIST = 0,
            LINELIST,
            LINESTRIP,
            TRIANGLELIST,
            TRIANGLESTRIP,
        };

        enum class FilterMode : UINT8
        {
            MIN_MAG_MIP_POINT = 0,
            MIN_MAG_POINT_MIP_LINEAR,
            MIN_POINT_MAG_LINEAR_MIP_POINT,
            MIN_POINT_MAG_MIP_LINEAR,
            MIN_LINEAR_MAG_MIP_POINT,
            MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            MIN_MAG_LINEAR_MIP_POINT,
            MIN_MAG_MIP_LINEAR,
            ANISOTROPIC,
        };

        enum class AddressMode : UINT8
        {
            CLAMP = 0,
            WRAP,
            MIRROR,
            MIRROR_ONCE,
            BORDER,
        };

        namespace BufferFlags
        {
            enum
            {
                VERTEX_BUFFER = 1 << 0,
                INDEX_BUFFER = 1 << 1,
                CONSTANT_BUFFER = 1 << 2,
                STRUCTURED_BUFFER = 1 << 3,
                RESOURCE = 1 << 4,
                UNORDERED_ACCESS = 1 << 5,
                STATIC = 1 << 6,
                DYNAMIC = 1 << 7,
            };
        }; // BufferFlags

        namespace TextureFlags
        {
            enum
            {
                RESOURCE = 1 << 0,
                UNORDERED_ACCESS = 1 << 1,
            };
        }; // TextureFlags

        namespace ClearMask
        {
            enum
            {
                DEPTH = 1 << 0,
                STENCIL = 1 << 1,
            };
        }; // ClearMask

        struct ViewPort
        {
            Math::Float2 position;
            Math::Float2 size;
            float nearDepth;
            float farDepth;
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
                : fillMode(FillMode::SOLID)
                , cullMode(CullMode::BACK)
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
                    : failOperation(StencilOperation::KEEP)
                    , depthFailOperation(StencilOperation::KEEP)
                    , passOperation(StencilOperation::KEEP)
                    , comparisonFunction(ComparisonFunction::ALWAYS)
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
                , writeMask(DepthWrite::ALL)
                , comparisonFunction(ComparisonFunction::ALWAYS)
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
                , colorSource(BlendSource::ONE)
                , colorDestination(BlendSource::ZERO)
                , colorOperation(BlendOperation::ADD)
                , alphaSource(BlendSource::ONE)
                , alphaDestination(BlendSource::ZERO)
                , alphaOperation(BlendOperation::ADD)
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
                : format(Format::UNKNOWN)
                , semanticName(nullptr)
                , semanticIndex(0)
                , slotClass(ElementType::VERTEX)
                , slotIndex(0)
            {
            }

            InputElement(Format format, LPCSTR semanticName, UINT32 semanticIndex, ElementType slotClass = ElementType::VERTEX, UINT32 slotIndex = 0)
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
                : filterMode(FilterMode::MIN_MAG_MIP_POINT)
                , addressModeU(AddressMode::CLAMP)
                , addressModeV(AddressMode::CLAMP)
                , addressModeW(AddressMode::CLAMP)
                , mipLevelBias(0.0f)
                , maximumAnisotropy(1)
                , comparisonFunction(ComparisonFunction::NEVER)
                , borderColor(0.0f, 0.0f, 0.0f, 1.0f)
                , minimumMipLevel(0.0f)
                , maximumMipLevel(Math::Infinity)
            {
            }
        };

        DECLARE_INTERFACE_IID(ContextInterface, "95262C77-0F56-4447-9337-5819E68B372E") : virtual public IUnknown
        {
            DECLARE_INTERFACE(SubSystemInterface)
            {
                STDMETHOD_(void, setProgram)                    (THIS_ Handle resourceHandle) PURE;
                STDMETHOD_(void, setConstantBuffer)             (THIS_ Handle resourceHandle, UINT32 stage) PURE;
                STDMETHOD_(void, setSamplerStates)              (THIS_ Handle resourceHandle, UINT32 stage) PURE;
                STDMETHOD_(void, setResource)                   (THIS_ Handle resourceHandle, UINT32 stage) PURE;
                STDMETHOD_(void, setUnorderedAccess)            (THIS_ Handle resourceHandle, UINT32 stage) { };
            };

            STDMETHOD_(SubSystemInterface *, getComputeSystem)  (THIS) PURE;
            STDMETHOD_(SubSystemInterface *, getVertexSystem)   (THIS) PURE;
            STDMETHOD_(SubSystemInterface *, getGeometrySystem) (THIS) PURE;
            STDMETHOD_(SubSystemInterface *, getPixelSystem)    (THIS) PURE;

            STDMETHOD_(void, clearResources)                    (THIS) PURE;

            STDMETHOD_(void, setViewports)                      (THIS_ const std::vector<ViewPort> &viewPortList) PURE;
            STDMETHOD_(void, setScissorRect)                    (THIS_ const std::vector<Rectangle<UINT32>> &rectangleList) PURE;

            STDMETHOD_(void, clearRenderTarget)                 (THIS_ Handle targetHandle, const Math::Float4 &color) PURE;
            STDMETHOD_(void, clearDepthStencilTarget)           (THIS_ Handle depthHandle, UINT32 flags, float depth, UINT32 stencil) PURE;
            STDMETHOD_(void, setRenderTargets)                  (THIS_ const std::vector<Handle> &targetHandleList, Handle depthHandle) PURE;

            STDMETHOD_(void, setRenderStates)                   (THIS_ Handle resourceHandle) PURE;
            STDMETHOD_(void, setDepthStates)                    (THIS_ Handle resourceHandle, UINT32 stencilReference) PURE;
            STDMETHOD_(void, setBlendStates)                    (THIS_ Handle resourceHandle, const Math::Float4 &blendFactor, UINT32 sampleMask) PURE;

            STDMETHOD_(void, setVertexBuffer)                   (THIS_ Handle resourceHandle, UINT32 slot, UINT32 offset) PURE;
            STDMETHOD_(void, setIndexBuffer)                    (THIS_ Handle resourceHandle, UINT32 offset) PURE;
            STDMETHOD_(void, setPrimitiveType)                  (THIS_ PrimitiveType type) PURE;

            STDMETHOD_(void, drawPrimitive)                     (THIS_ UINT32 vertexCount, UINT32 firstVertex) PURE;
            STDMETHOD_(void, drawInstancedPrimitive)            (THIS_ UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex) PURE;

            STDMETHOD_(void, drawIndexedPrimitive)              (THIS_ UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) PURE;
            STDMETHOD_(void, drawInstancedIndexedPrimitive)     (THIS_ UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex) PURE;

            STDMETHOD_(void, dispatch)                          (THIS_ UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ) PURE;

            STDMETHOD_(void, finishCommandList)                 (THIS_ IUnknown **instance) PURE;
        };

        DECLARE_INTERFACE_IID(Interface, "CA9BBC81-83E9-4C26-9BED-5BF3B2D189D6") : virtual public IUnknown
        {
            STDMETHOD(initialize)                               (THIS_ HWND window, bool windowed, UINT32 width, UINT32 height, Format depthBufferFormat = Format::UNKNOWN) PURE;
            STDMETHOD(resize)                                   (THIS_ bool windowed, UINT32 width, UINT32 height, Format depthBufferFormat = Format::UNKNOWN) PURE;

            STDMETHOD_(UINT32, getWidth)                        (THIS) PURE;
            STDMETHOD_(UINT32, getHeight)                       (THIS) PURE;
            STDMETHOD_(bool, isWindowed)                        (THIS) PURE;

            STDMETHOD_(Video2D::Interface *, getVideo2D)        (THIS) PURE;
            STDMETHOD_(ContextInterface *, getDefaultContext)   (THIS) PURE;
            STDMETHOD(createDeferredContext)                    (THIS_ ContextInterface **instance) PURE;

            STDMETHOD_(void, freeResourcePool)                  (THIS_ Handle resourcePoolHandle) PURE;
            STDMETHOD_(void, freeAllResources)                  (THIS) PURE;

            STDMETHOD_(Handle, createEvent)                     (THIS_ Handle resourcePoolHandle) PURE;
            STDMETHOD_(void, setEvent)                          (THIS_ Handle resourceHandle) PURE;
            STDMETHOD_(bool, isEventSet)                        (THIS_ Handle resourceHandle) PURE;

            STDMETHOD_(Handle, createRenderStates)              (THIS_ Handle resourcePoolHandle, const RenderStates &kStates) PURE;
            STDMETHOD_(Handle, createDepthStates)               (THIS_ Handle resourcePoolHandle, const DepthStates &kStates) PURE;
            STDMETHOD_(Handle, createBlendStates)               (THIS_ Handle resourcePoolHandle, const UnifiedBlendStates &kStates) PURE;
            STDMETHOD_(Handle, createBlendStates)               (THIS_ Handle resourcePoolHandle, const IndependentBlendStates &kStates) PURE;
            STDMETHOD_(Handle, createSamplerStates)             (THIS_ Handle resourcePoolHandle, const SamplerStates &kStates) PURE;

            STDMETHOD_(Handle, createRenderTarget)              (THIS_ Handle resourcePoolHandle, UINT32 width, UINT32 height, Format format) PURE;
            STDMETHOD_(Handle, createDepthTarget)               (THIS_ Handle resourcePoolHandle, UINT32 width, UINT32 height, Format format) PURE;

            STDMETHOD_(Handle, createBuffer)                    (THIS_ Handle resourcePoolHandle, UINT32 stride, UINT32 count, UINT32 flags, LPCVOID staticData = nullptr) PURE;
            STDMETHOD_(Handle, createBuffer)                    (THIS_ Handle resourcePoolHandle, Format format, UINT32 count, UINT32 flags, LPCVOID staticData = nullptr) PURE;
            STDMETHOD_(void, updateBuffer)                      (THIS_ Handle resourceHandle, LPCVOID data) PURE;
            STDMETHOD(mapBuffer)                                (THIS_ Handle resourceHandle, LPVOID *data) PURE;
            STDMETHOD_(void, unmapBuffer)                       (THIS_ Handle resourceHandle) PURE;

            STDMETHOD_(Handle, compileComputeProgram)           (THIS_ Handle resourcePoolHandle, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, compileVertexProgram)            (THIS_ Handle resourcePoolHandle, LPCSTR programScript, LPCSTR entryFunction, const std::vector<InputElement> &elementLayout, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, compileGeometryProgram)          (THIS_ Handle resourcePoolHandle, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, compilePixelProgram)             (THIS_ Handle resourcePoolHandle, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

            STDMETHOD_(Handle, loadComputeProgram)              (THIS_ Handle resourcePoolHandle, LPCWSTR fileName, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, loadVertexProgram)               (THIS_ Handle resourcePoolHandle, LPCWSTR fileName, LPCSTR entryFunction, const std::vector<InputElement> &elementLayout, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, loadGeometryProgram)             (THIS_ Handle resourcePoolHandle, LPCWSTR fileName, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
            STDMETHOD_(Handle, loadPixelProgram)                (THIS_ Handle resourcePoolHandle, LPCWSTR fileName, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

            STDMETHOD_(Handle, createTexture)                   (THIS_ Handle resourcePoolHandle, UINT32 width, UINT32 height, UINT32 depth, UINT8 format, UINT32 flags) PURE;
            STDMETHOD_(Handle, loadTexture)                     (THIS_ Handle resourcePoolHandle, LPCWSTR fileName, UINT32 flags) PURE;
            STDMETHOD_(void, updateTexture)                     (THIS_ Handle resourceHandle, LPCVOID data, UINT32 pitch, Rectangle<UINT32> *rectangle = nullptr) PURE;

            STDMETHOD_(void, clearDefaultRenderTarget)          (THIS_ const Math::Float4 &color) PURE;
            STDMETHOD_(void, clearDefaultDepthStencilTarget)    (THIS_ UINT32 flags, float depth, UINT32 stencil) PURE;
            STDMETHOD_(void, setDefaultTargets)                 (THIS_ ContextInterface *context = nullptr, Handle depthHandle = InvalidHandle) PURE;

            STDMETHOD_(void, executeCommandList)                (THIS_ IUnknown *commandList) PURE;

            STDMETHOD_(void, present)                           (THIS_ bool waitForVerticalSync) PURE;
        };
    }; // namespace Video3D
}; // namespace Gek
