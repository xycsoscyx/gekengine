#pragma warning(disable : 4005)

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\COM.h"
#include "GEK\Context\UnknownMixin.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\System\VideoSystem.h"
#include <atlbase.h>
#include <atlpath.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <wincodec.h>
#include <algorithm>
#include <memory>
#include <ppl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace Gek
{
    namespace DirectX
    {
        // Both these lists must match, since the same GEK Format can be used for either textures or buffers
        // The size list must also match
        static const DXGI_FORMAT TextureFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_D16_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            DXGI_FORMAT_D32_FLOAT,
        };

        static_assert(ARRAYSIZE(TextureFormatList) == static_cast<UINT8>(Video::Format::NumFormats), "New format added without adding to all TextureFormatList.");

        static const DXGI_FORMAT BufferFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R8_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
        };

        static_assert(ARRAYSIZE(BufferFormatList) == static_cast<UINT8>(Video::Format::NumFormats), "New format added without adding to all BufferFormatList.");

        static const UINT32 FormatStrideList[] =
        {
            0,
            sizeof(UINT8),
            (sizeof(UINT8) * 2),
            (sizeof(UINT8) * 4),
            (sizeof(UINT8) * 4),
            (sizeof(UINT8) * 4),
            (sizeof(UINT8) * 4),
            sizeof(UINT16),
            (sizeof(UINT16) * 2),
            (sizeof(UINT16) * 4),
            sizeof(UINT32),
            (sizeof(UINT32) * 2),
            (sizeof(UINT32) * 3),
            (sizeof(UINT32) * 4),
            (sizeof(float) / 2),
            ((sizeof(float) / 2) * 3),
            ((sizeof(float) / 2) * 2),
            ((sizeof(float) / 2) * 4),
            sizeof(float),
            (sizeof(float) * 2),
            (sizeof(float) * 3),
            (sizeof(float) * 4),
            sizeof(UINT16),
            sizeof(UINT32),
            sizeof(UINT32),
        };

        static_assert(ARRAYSIZE(FormatStrideList) == static_cast<UINT8>(Video::Format::NumFormats), "New format added without adding to all FormatStrideList.");

        static const D3D11_DEPTH_WRITE_MASK DepthWriteMaskList[] =
        {
            D3D11_DEPTH_WRITE_MASK_ZERO,
            D3D11_DEPTH_WRITE_MASK_ALL,
        };

        static const D3D11_TEXTURE_ADDRESS_MODE AddressModeList[] =
        {
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
            D3D11_TEXTURE_ADDRESS_BORDER,
        };

        static const D3D11_COMPARISON_FUNC ComparisonFunctionList[] =
        {
            D3D11_COMPARISON_ALWAYS,
            D3D11_COMPARISON_NEVER,
            D3D11_COMPARISON_EQUAL,
            D3D11_COMPARISON_NOT_EQUAL,
            D3D11_COMPARISON_LESS,
            D3D11_COMPARISON_LESS_EQUAL,
            D3D11_COMPARISON_GREATER,
            D3D11_COMPARISON_GREATER_EQUAL,
        };

        static const D3D11_STENCIL_OP StencilOperationList[] =
        {
            D3D11_STENCIL_OP_ZERO,
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_REPLACE,
            D3D11_STENCIL_OP_INVERT,
            D3D11_STENCIL_OP_INCR,
            D3D11_STENCIL_OP_INCR_SAT,
            D3D11_STENCIL_OP_DECR,
            D3D11_STENCIL_OP_DECR_SAT,
        };

        static const D3D11_BLEND BlendSourceList[] =
        {
            D3D11_BLEND_ZERO,
            D3D11_BLEND_ONE,
            D3D11_BLEND_BLEND_FACTOR,
            D3D11_BLEND_INV_BLEND_FACTOR,
            D3D11_BLEND_SRC_COLOR,
            D3D11_BLEND_INV_SRC_COLOR,
            D3D11_BLEND_SRC_ALPHA,
            D3D11_BLEND_INV_SRC_ALPHA,
            D3D11_BLEND_SRC_ALPHA_SAT,
            D3D11_BLEND_DEST_COLOR,
            D3D11_BLEND_INV_DEST_COLOR,
            D3D11_BLEND_DEST_ALPHA,
            D3D11_BLEND_INV_DEST_ALPHA,
            D3D11_BLEND_SRC1_COLOR,
            D3D11_BLEND_INV_SRC1_COLOR,
            D3D11_BLEND_SRC1_ALPHA,
            D3D11_BLEND_INV_SRC1_ALPHA,
        };

        static const D3D11_BLEND_OP BlendOperationList[] =
        {
            D3D11_BLEND_OP_ADD,
            D3D11_BLEND_OP_SUBTRACT,
            D3D11_BLEND_OP_REV_SUBTRACT,
            D3D11_BLEND_OP_MIN,
            D3D11_BLEND_OP_MAX,
        };

        static const D3D11_FILL_MODE FillModeList[] =
        {
            D3D11_FILL_WIREFRAME,
            D3D11_FILL_SOLID,
        };

        static const D3D11_CULL_MODE CullModeList[] =
        {
            D3D11_CULL_NONE,
            D3D11_CULL_FRONT,
            D3D11_CULL_BACK,
        };

        static const D3D11_FILTER FilterList[] =
        {
            D3D11_FILTER_MIN_MAG_MIP_POINT,
            D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
            D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
            D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            D3D11_FILTER_ANISOTROPIC,
        };

        static const D3D11_PRIMITIVE_TOPOLOGY TopologList[] =
        {
            D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

        static const D3D11_MAP MapList[] =
        {
            D3D11_MAP_READ,
            D3D11_MAP_WRITE,
            D3D11_MAP_READ_WRITE,
            D3D11_MAP_WRITE_DISCARD,
            D3D11_MAP_WRITE_NO_OVERWRITE,
        };
    }; // namespace DirectX

    class VertexProgram : public UnknownMixin
    {
    private:
        CComPtr<ID3D11VertexShader> d3dVertexShader;
        CComPtr<ID3D11InputLayout> d3dInputLayout;

    public:
        VertexProgram(ID3D11VertexShader *d3dVertexShader, ID3D11InputLayout *d3dInputLayout)
            : d3dVertexShader(d3dVertexShader)
            , d3dInputLayout(d3dInputLayout)
        {
        }

        BEGIN_INTERFACE_LIST(VertexProgram)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11VertexShader, d3dVertexShader)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11InputLayout, d3dInputLayout)
        END_INTERFACE_LIST_UNKNOWN
    };

    class BufferImplementation : public UnknownMixin
        , public VideoBuffer
    {
    private:
        CComPtr<ID3D11Buffer> d3dBuffer;
        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
        Video::Format format;
        UINT32 stride;
        UINT32 count;

    public:
        BufferImplementation(Video::Format format, UINT32 stride, UINT32 count, ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView = nullptr, ID3D11UnorderedAccessView *d3dUnorderedAccessView = nullptr)
            : format(format)
            , stride(stride)
            , count(count)
            , d3dBuffer(d3dBuffer)
            , d3dShaderResourceView(d3dShaderResourceView)
            , d3dUnorderedAccessView(d3dUnorderedAccessView)
        {
        }

        BEGIN_INTERFACE_LIST(BufferImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoBuffer)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Resource, d3dBuffer)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Buffer, d3dBuffer)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, d3dShaderResourceView)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, d3dUnorderedAccessView)
        END_INTERFACE_LIST_UNKNOWN

        // VideoBuffer
        STDMETHODIMP_(Video::Format) getFormat(void)
        {
            return format;
        }

        STDMETHODIMP_(UINT32) getStride(void)
        {
            return stride;
        }

        STDMETHODIMP_(UINT32) getCount(void)
        {
            return stride;
        }
    };

    class TextureImplementation : public UnknownMixin
        , public VideoTarget
    {
    protected:
        CComPtr<ID3D11Resource> d3dResource;
        CComPtr<ID3D11View> d3dResourceView;
        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
        Video::Format format;
        UINT32 width;
        UINT32 height;
        UINT32 depth;
        Video::ViewPort viewPort;

    public:
        TextureImplementation(ID3D11Resource *d3dResource, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : d3dResource(d3dResource)
            , d3dShaderResourceView(d3dShaderResourceView)
            , d3dUnorderedAccessView(d3dUnorderedAccessView)
            , format(format)
            , width(width)
            , height(height)
            , depth(depth)
        {
        }

        TextureImplementation(ID3D11Resource *d3dResource, ID3D11View *d3dResourceView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height)
            : d3dResource(d3dResource)
            , d3dResourceView(d3dResourceView)
            , d3dShaderResourceView(d3dShaderResourceView)
            , d3dUnorderedAccessView(d3dUnorderedAccessView)
            , format(format)
            , width(width)
            , height(height)
            , depth(1)
            , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(width), float(height)), 0.0f, 1.0f)
        {
        }

        BEGIN_INTERFACE_LIST(TextureImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoTarget)
            INTERFACE_LIST_ENTRY_COM(VideoTexture)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Resource, d3dResource)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, d3dResourceView)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DepthStencilView, d3dResourceView)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, d3dShaderResourceView)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, d3dUnorderedAccessView)
        END_INTERFACE_LIST_UNKNOWN

        // VideoTexture
        STDMETHODIMP_(Video::Format) getFormat(void)
        {
            return format;
        }

        STDMETHODIMP_(UINT32) getWidth(void)
        {
            return width;
        }

        STDMETHODIMP_(UINT32) getHeight(void)
        {
            return height;
        }

        STDMETHODIMP_(UINT32) getDepth(void)
        {
            return depth;
        }

        // VideoTarget
        STDMETHODIMP_(const Video::ViewPort &) getViewPort(void)
        {
            return viewPort;
        }
    };

    class IncludeImplementation : public UnknownMixin
        , public ID3DInclude
    {
    private:
        CPathW shaderFilePath;
        std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude;
        std::vector<UINT8> includeBuffer;

    public:
        IncludeImplementation(const CStringW &shaderFileName, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude)
            : shaderFilePath(shaderFileName)
            , onInclude(onInclude)
        {
            shaderFilePath.RemoveFileSpec();
        }

        BEGIN_INTERFACE_LIST(IncludeImplementation)
            INTERFACE_LIST_ENTRY_COM(IUnknown)
        END_INTERFACE_LIST_UNKNOWN

        // ID3DInclude
        STDMETHODIMP Open(D3D_INCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize)
        {
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);
            GEK_REQUIRE_RETURN(data, E_INVALIDARG);
            GEK_REQUIRE_RETURN(dataSize, E_INVALIDARG);

            includeBuffer.clear();
            HRESULT resultValue = Gek::FileSystem::load(CA2W(fileName), includeBuffer);
            if (FAILED(resultValue))
            {
                CPathW shaderPath;
                shaderPath.Combine(shaderFilePath, CA2W(fileName));
                resultValue = Gek::FileSystem::load(shaderPath, includeBuffer);
            }

            if (FAILED(resultValue))
            {
                resultValue = onInclude(fileName, includeBuffer);
            }

            if (SUCCEEDED(resultValue))
            {
                (*data) = includeBuffer.data();
                (*dataSize) = includeBuffer.size();
            }

            return resultValue;
        }

        STDMETHODIMP Close(LPCVOID data)
        {
            return S_OK;
        }
    };

    class PipelineImplementation : public UnknownMixin
    {
    protected:
        ID3D11DeviceContext *d3dDeviceContext;

    public:
        PipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : d3dDeviceContext(d3dDeviceContext)
        {
        }
    };

    class ComputePipelineImplementation : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        ComputePipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        BEGIN_INTERFACE_LIST(ComputePipelineImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoPipeline)
        END_INTERFACE_LIST_UNKNOWN

        // VideoPipeline
        STDMETHODIMP_(void) setProgram(IUnknown *program)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ComputeShader> d3dComputeShader(program);
            d3dDeviceContext->CSSetShader(d3dComputeShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->CSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->CSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
            d3dDeviceContext->CSSetShaderResources(stage, 1, &d3dShaderResourceView.p);
        }

        STDMETHODIMP_(void) setUnorderedAccess(IUnknown *unorderedAccess, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView(unorderedAccess);
            d3dDeviceContext->CSSetUnorderedAccessViews(stage, 1, &d3dUnorderedAccessView.p, nullptr);
        }
    };

    class VertexPipelineImplementation : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        VertexPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        BEGIN_INTERFACE_LIST(VertexPipelineImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoPipeline)
        END_INTERFACE_LIST_UNKNOWN

        // VideoPipeline
        STDMETHODIMP_(void) setProgram(IUnknown *program)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);

            CComQIPtr<ID3D11VertexShader> d3dVertexShader(program);
            d3dDeviceContext->VSSetShader(d3dVertexShader, nullptr, 0);

            CComQIPtr<ID3D11InputLayout> d3dInputLayout(program);
            d3dDeviceContext->IASetInputLayout(d3dInputLayout);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->VSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->VSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
            d3dDeviceContext->VSSetShaderResources(stage, 1, &d3dShaderResourceView.p);
        }
    };

    class GeometryPipelineImplementation : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        GeometryPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        BEGIN_INTERFACE_LIST(GeometryPipelineImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoPipeline)
        END_INTERFACE_LIST_UNKNOWN

        // VideoPipeline
        STDMETHODIMP_(void) setProgram(IUnknown *program)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11GeometryShader> d3dGeometryShader(program);
            d3dDeviceContext->GSSetShader(d3dGeometryShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->GSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->GSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
            d3dDeviceContext->GSSetShaderResources(stage, 1, &d3dShaderResourceView.p);
        }
    };

    class PixelPipelineImplementation : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        PixelPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        BEGIN_INTERFACE_LIST(PixelPipelineImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoPipeline)
        END_INTERFACE_LIST_UNKNOWN

        // VideoPipeline
        STDMETHODIMP_(void) setProgram(IUnknown *program)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11PixelShader> d3dPixelShader(program);
            d3dDeviceContext->PSSetShader(d3dPixelShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->PSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->PSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
            d3dDeviceContext->PSSetShaderResources(stage, 1, &d3dShaderResourceView.p);
        }
    };

    class VideoContextImplementation : public Gek::ContextUserMixin
        , public VideoContext
    {
    protected:
        CComPtr<ID3D11DeviceContext> d3dDeviceContext;
        CComPtr<VideoPipeline> computeSystemHandler;
        CComPtr<VideoPipeline> vertexSystemHandler;
        CComPtr<VideoPipeline> geomtrySystemHandler;
        CComPtr<VideoPipeline> pixelSystemHandler;

    public:
        VideoContextImplementation(void)
        {
        }

        VideoContextImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : d3dDeviceContext(d3dDeviceContext)
            , computeSystemHandler(new ComputePipelineImplementation(d3dDeviceContext))
            , vertexSystemHandler(new VertexPipelineImplementation(d3dDeviceContext))
            , geomtrySystemHandler(new GeometryPipelineImplementation(d3dDeviceContext))
            , pixelSystemHandler(new PixelPipelineImplementation(d3dDeviceContext))
        {
        }

        BEGIN_INTERFACE_LIST(VideoContextImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoContext)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DeviceContext, d3dDeviceContext)
            END_INTERFACE_LIST_USER

        // VideoContext
        STDMETHODIMP_(VideoPipeline *) computePipeline(void)
        {
            GEK_REQUIRE_RETURN(computeSystemHandler, nullptr);
            return computeSystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) vertexPipeline(void)
        {
            GEK_REQUIRE_RETURN(vertexSystemHandler, nullptr);
            return vertexSystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) geometryPipeline(void)
        {
            GEK_REQUIRE_RETURN(geomtrySystemHandler, nullptr);
            return geomtrySystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) pixelPipeline(void)
        {
            GEK_REQUIRE_RETURN(pixelSystemHandler, nullptr);
            return pixelSystemHandler.p;
        }

        STDMETHODIMP_(void) generateMipMaps(VideoTexture *texture)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(texture);

            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(texture);
            if (d3dShaderResourceView)
            {
                d3dDeviceContext->GenerateMips(d3dShaderResourceView);
            }
        }

        STDMETHODIMP_(void) clearResources(void)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            
            // ClearState clears too much, we only want to clear resource between calls, but leave constants/states/modes alone
            //d3dDeviceContext->ClearState();

            static ID3D11ShaderResourceView *const d3dShaderResourceViewList[10] = { nullptr };
            static ID3D11UnorderedAccessView *const d3dUnorderedAccessViewList[7] = { nullptr };
            static ID3D11RenderTargetView  *const d3dRenderTargetViewList[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

            d3dDeviceContext->CSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->CSSetUnorderedAccessViews(0, 7, d3dUnorderedAccessViewList, nullptr);
            d3dDeviceContext->VSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->GSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->PSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, d3dRenderTargetViewList, nullptr);
        }

        STDMETHODIMP_(void) setViewports(Video::ViewPort *viewPortList, UINT32 viewPortCount)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(viewPortList);
            GEK_REQUIRE_VOID_RETURN(viewPortCount);
            d3dDeviceContext->RSSetViewports(viewPortCount, (D3D11_VIEWPORT *)viewPortList);
        }

        STDMETHODIMP_(void) setScissorRect(Shapes::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(rectangleList);
            GEK_REQUIRE_VOID_RETURN(rectangleCount);
            d3dDeviceContext->RSSetScissorRects(rectangleCount, (D3D11_RECT *)rectangleList);
        }

        STDMETHODIMP_(void) clearRenderTarget(VideoTarget *renderTarget, const Math::Color &colorClear)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11RenderTargetView> d3dRenderTargetView(renderTarget);
            if (d3dRenderTargetView)
            {
                d3dDeviceContext->ClearRenderTargetView(d3dRenderTargetView, colorClear.data);
            }
        }

        STDMETHODIMP_(void) clearDepthStencilTarget(IUnknown *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11DepthStencilView> d3dDepthStencilView(depthBuffer);
            if (d3dDepthStencilView)
            {
                d3dDeviceContext->ClearDepthStencilView(d3dDepthStencilView,
                    ((flags & Video::ClearMask::Depth ? D3D11_CLEAR_DEPTH : 0) |
                     (flags & Video::ClearMask::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                    depthClear, stencilClear);
            }
        }

        STDMETHODIMP_(void) clearUnorderedAccessBuffer(VideoBuffer *buffer, float value)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(buffer);

            CComQIPtr<ID3D11UnorderedAccessView> d3dBuffer(buffer);
            if (buffer)
            {
                float valueList[4] =
                {
                    value,
                    value,
                    value,
                    value,
                };

                d3dDeviceContext->ClearUnorderedAccessViewFloat(d3dBuffer, valueList);
            }
        }

        ID3D11RenderTargetView *d3dRenderTargetViewList[8];
        STDMETHODIMP_(void) setRenderTargets(VideoTarget **renderTargetList, UINT32 renderTargetCount, IUnknown *depthBuffer)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(renderTargetList);

            for (UINT32 renderTarget = 0; renderTarget < renderTargetCount; renderTarget++)
            {
                CComQIPtr<ID3D11RenderTargetView> d3dRenderTarget(renderTargetList[renderTarget]);
                d3dRenderTargetViewList[renderTarget] = d3dRenderTarget;
            }

            CComQIPtr<ID3D11DepthStencilView> d3dDepthBuffer(depthBuffer);
            d3dDeviceContext->OMSetRenderTargets(renderTargetCount, d3dRenderTargetViewList, d3dDepthBuffer);
        }

        STDMETHODIMP_(void) setRenderStates(IUnknown *renderStates)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11RasterizerState> d3dRasterizerState(renderStates);
            d3dDeviceContext->RSSetState(d3dRasterizerState);
        }

        STDMETHODIMP_(void) setDepthStates(IUnknown *depthStates, UINT32 stencilReference)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11DepthStencilState> d3dDepthStencilState(depthStates);
            d3dDeviceContext->OMSetDepthStencilState(d3dDepthStencilState, stencilReference);
        }

        STDMETHODIMP_(void) setBlendStates(IUnknown *blendStates, const Math::Color &blendFactor, UINT32 mask)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11BlendState> d3dBlendState(blendStates);
            d3dDeviceContext->OMSetBlendState(d3dBlendState, blendFactor.data, mask);
        }

        STDMETHODIMP_(void) setVertexBuffer(UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(vertexBuffer);
            UINT32 stride = (vertexBuffer ? vertexBuffer->getStride() : 0);
            d3dDeviceContext->IASetVertexBuffers(slot, 1, &d3dBuffer.p, &stride, &offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(VideoBuffer *indexBuffer, UINT32 offset)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(indexBuffer);
            DXGI_FORMAT format = (indexBuffer ? (indexBuffer->getFormat() == Video::Format::Short ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT) : DXGI_FORMAT_UNKNOWN);
            d3dDeviceContext->IASetIndexBuffer(d3dBuffer, format, offset);
        }

        STDMETHODIMP_(void) setPrimitiveType(Video::PrimitiveType primitiveType)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->IASetPrimitiveTopology(DirectX::TopologList[static_cast<UINT8>(primitiveType)]);
        }

        STDMETHODIMP_(void) drawPrimitive(UINT32 vertexCount, UINT32 firstVertex)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->Draw(vertexCount, firstVertex);
        }

        STDMETHODIMP_(void) drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        STDMETHODIMP_(void) drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
        }

        STDMETHODIMP_(void) drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
        }

        STDMETHODIMP_(void) dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        STDMETHODIMP_(void) finishCommandList(IUnknown **returnObject)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext && returnObject);

            CComPtr<ID3D11CommandList> d3dCommandList;
            d3dDeviceContext->FinishCommandList(FALSE, &d3dCommandList);
            if (d3dCommandList)
            {
                d3dCommandList->QueryInterface(IID_PPV_ARGS(returnObject));
            }
        }
    };

    class VideoSystemImplementation : virtual public VideoContextImplementation
        , virtual public VideoSystem
    {
    private:
        HWND window;
        bool isChildWindow;
        bool fullScreen;
        Video::Format format;

        CComPtr<ID3D11Device> d3dDevice;
        CComPtr<IDXGISwapChain> dxSwapChain;
        CComPtr<VideoTarget> backBuffer;

    public:
        VideoSystemImplementation(void)
            : window(nullptr)
            , isChildWindow(false)
            , fullScreen(false)
            , format(Video::Format::Unknown)
        {
        }

        ~VideoSystemImplementation(void)
        {
            setFullScreen(false);
            backBuffer.Release();
            dxSwapChain.Release();
            d3dDevice.Release();
        }

        BEGIN_INTERFACE_LIST(VideoSystemImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoSystem)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, d3dDevice)
            INTERFACE_LIST_ENTRY_MEMBER(IID_IDXGISwapChain, dxSwapChain)
        END_INTERFACE_LIST_USER

        // VideoSystem
        STDMETHODIMP initialize(HWND window, bool fullScreen, Video::Format format)
        {
            GEK_TRACE_FUNCTION(VideoSystem);

            GEK_REQUIRE_RETURN(window, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            this->format = format;
            this->isChildWindow = (GetParent(window) != nullptr);
            this->fullScreen = (isChildWindow ? false : fullScreen);
            this->window = window;

            DXGI_SWAP_CHAIN_DESC swapChainDescription;
            swapChainDescription.BufferDesc.Width = 0;
            swapChainDescription.BufferDesc.Height = 0;
            swapChainDescription.BufferDesc.Format = DirectX::TextureFormatList[static_cast<UINT8>(format)];
            swapChainDescription.BufferDesc.RefreshRate.Numerator = 60;
            swapChainDescription.BufferDesc.RefreshRate.Denominator = 1;
            swapChainDescription.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            swapChainDescription.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            swapChainDescription.SampleDesc.Count = 1;
            swapChainDescription.SampleDesc.Quality = 0;
            swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDescription.BufferCount = 1;
            swapChainDescription.OutputWindow = window;
            swapChainDescription.Windowed = !fullScreen;
            swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

            UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            D3D_FEATURE_LEVEL featureLevelList[] =
            {
                D3D_FEATURE_LEVEL_11_0,
            };

            D3D_FEATURE_LEVEL featureLevel;
            resultValue = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, _ARRAYSIZE(featureLevelList), D3D11_SDK_VERSION, &swapChainDescription, &dxSwapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);
            if (SUCCEEDED(resultValue))
            {
                computeSystemHandler = new ComputePipelineImplementation(d3dDeviceContext);
                vertexSystemHandler = new VertexPipelineImplementation(d3dDeviceContext);
                geomtrySystemHandler = new GeometryPipelineImplementation(d3dDeviceContext);
                pixelSystemHandler = new PixelPipelineImplementation(d3dDeviceContext);
            }

            if (SUCCEEDED(resultValue) && fullScreen && !isChildWindow)
            {
                resultValue = dxSwapChain->SetFullscreenState(true, nullptr);
            }

            return resultValue;
        }

        STDMETHODIMP setFullScreen(bool fullScreen)
        {
            HRESULT resultValue = E_FAIL;
            if (!isChildWindow && this->fullScreen != fullScreen)
            {
                this->fullScreen = fullScreen;
                resultValue = dxSwapChain->SetFullscreenState(fullScreen, nullptr);
            }

            return resultValue;
        }

        STDMETHODIMP setSize(UINT32 width, UINT32 height, Video::Format format)
        {
            GEK_REQUIRE_RETURN(dxSwapChain, E_FAIL);

            HRESULT resultValue = E_FAIL;

            DXGI_SWAP_CHAIN_DESC chainDescription;
            DXGI_MODE_DESC &modeDescription = chainDescription.BufferDesc;
            dxSwapChain->GetDesc(&chainDescription);
            if (width != modeDescription.Width ||
                height != modeDescription.Height ||
                DirectX::TextureFormatList[static_cast<UINT8>(format)] != modeDescription.Format)
            {
                this->format = format;

                DXGI_MODE_DESC description;
                description.Width = width;
                description.Height = height;
                description.Format = DirectX::TextureFormatList[static_cast<UINT8>(format)];
                description.RefreshRate.Numerator = 60;
                description.RefreshRate.Denominator = 1;
                description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                resultValue = dxSwapChain->ResizeTarget(&description);
            }

            return resultValue;
        }

        STDMETHODIMP resize(void)
        {
            GEK_REQUIRE_RETURN(dxSwapChain, E_FAIL);

            backBuffer.Release();
            HRESULT resultValue = dxSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            return resultValue;
        }

        STDMETHODIMP_(VideoTarget *) getBackBuffer(void)
        {
            if (!backBuffer)
            {
                CComPtr<ID3D11Texture2D> d3dRenderTarget;
                HRESULT resultValue = dxSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                if (SUCCEEDED(resultValue) && d3dRenderTarget)
                {
                    CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                    resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                    if (SUCCEEDED(resultValue) && d3dRenderTargetView)
                    {
                        D3D11_TEXTURE2D_DESC description;
                        d3dRenderTarget->GetDesc(&description);
                        backBuffer = new TextureImplementation(d3dRenderTarget, d3dRenderTargetView, nullptr, nullptr, format, description.Width, description.Height);
                    }
                }
            }

            return backBuffer;
        }

        STDMETHODIMP_(VideoContext *) getDefaultContext(void)
        {
            return static_cast<VideoContext *>(this);
        }

        STDMETHODIMP_(bool) isFullScreen(void)
        {
            return fullScreen;
        }

        STDMETHODIMP createDeferredContext(VideoContext **returnObject)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_FAIL);
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
            resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext);
            if (SUCCEEDED(resultValue) && d3dDeferredDeviceContext)
            {
                resultValue = E_OUTOFMEMORY;
                CComPtr<VideoContextImplementation> deferredContext(new VideoContextImplementation(d3dDeferredDeviceContext));
                if (deferredContext)
                {
                    resultValue = deferredContext->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }

            return resultValue;
        }

        STDMETHODIMP createEvent(IUnknown **returnObject)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_QUERY_DESC description;
            description.Query = D3D11_QUERY_EVENT;
            description.MiscFlags = 0;

            CComPtr<ID3D11Query> d3dQuery;
            resultValue = d3dDevice->CreateQuery(&description, &d3dQuery);
            if (SUCCEEDED(resultValue) && d3dQuery)
            {
                resultValue = d3dQuery->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP_(void) setEvent(IUnknown *event)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Query> d3dQuery(event);
            d3dDeviceContext->End(d3dQuery);
        }

        STDMETHODIMP_(bool) isEventSet(IUnknown *event)
        {
            GEK_REQUIRE_RETURN(d3dDeviceContext, false);

            UINT32 isEventSet = 0;
            CComQIPtr<ID3D11Query> d3dQuery(event);
            if (d3dQuery)
            {
                if (FAILED(d3dDeviceContext->GetData(d3dQuery, (LPVOID)&isEventSet, sizeof(UINT32), TRUE)))
                {
                    isEventSet = 0;
                }
            }

            return (isEventSet == 1);
        }

        STDMETHODIMP createRenderStates(IUnknown **returnObject, const Video::RenderStates &renderStates)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_RASTERIZER_DESC rasterizerDescription;
            rasterizerDescription.FrontCounterClockwise = renderStates.frontCounterClockwise;
            rasterizerDescription.DepthBias = renderStates.depthBias;
            rasterizerDescription.DepthBiasClamp = renderStates.depthBiasClamp;
            rasterizerDescription.SlopeScaledDepthBias = renderStates.slopeScaledDepthBias;
            rasterizerDescription.DepthClipEnable = renderStates.depthClipEnable;
            rasterizerDescription.ScissorEnable = renderStates.scissorEnable;
            rasterizerDescription.MultisampleEnable = renderStates.multisampleEnable;
            rasterizerDescription.AntialiasedLineEnable = renderStates.antialiasedLineEnable;
            rasterizerDescription.FillMode = DirectX::FillModeList[static_cast<UINT8>(renderStates.fillMode)];
            rasterizerDescription.CullMode = DirectX::CullModeList[static_cast<UINT8>(renderStates.cullMode)];

            CComPtr<ID3D11RasterizerState> d3dStates;
            resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates);
            if (SUCCEEDED(resultValue) && d3dStates)
            {
                resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP createDepthStates(IUnknown **returnObject, const Video::DepthStates &depthStates)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
            depthStencilDescription.DepthEnable = depthStates.enable;
            depthStencilDescription.DepthFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthStates.comparisonFunction)];
            depthStencilDescription.StencilEnable = depthStates.stencilEnable;
            depthStencilDescription.StencilReadMask = depthStates.stencilReadMask;
            depthStencilDescription.StencilWriteMask = depthStates.stencilWriteMask;
            depthStencilDescription.FrontFace.StencilFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.failOperation)];
            depthStencilDescription.FrontFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.depthFailOperation)];
            depthStencilDescription.FrontFace.StencilPassOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.passOperation)];
            depthStencilDescription.FrontFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthStates.stencilFrontStates.comparisonFunction)];
            depthStencilDescription.BackFace.StencilFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.failOperation)];
            depthStencilDescription.BackFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.depthFailOperation)];
            depthStencilDescription.BackFace.StencilPassOp = DirectX::StencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.passOperation)];
            depthStencilDescription.BackFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthStates.stencilBackStates.comparisonFunction)];
            depthStencilDescription.DepthWriteMask = DirectX::DepthWriteMaskList[static_cast<UINT8>(depthStates.writeMask)];

            CComPtr<ID3D11DepthStencilState> d3dStates;
            resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates);
            if (SUCCEEDED(resultValue) && d3dStates)
            {
                resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_BLEND_DESC blendDescription;
            blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
            blendDescription.IndependentBlendEnable = false;
            blendDescription.RenderTarget[0].BlendEnable = blendStates.enable;
            blendDescription.RenderTarget[0].SrcBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.colorSource)];
            blendDescription.RenderTarget[0].DestBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.colorDestination)];
            blendDescription.RenderTarget[0].BlendOp = DirectX::BlendOperationList[static_cast<UINT8>(blendStates.colorOperation)];
            blendDescription.RenderTarget[0].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.alphaSource)];
            blendDescription.RenderTarget[0].DestBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.alphaDestination)];
            blendDescription.RenderTarget[0].BlendOpAlpha = DirectX::BlendOperationList[static_cast<UINT8>(blendStates.alphaOperation)];
            blendDescription.RenderTarget[0].RenderTargetWriteMask = 0;
            if (blendStates.writeMask & Video::ColorMask::R)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
            }

            if (blendStates.writeMask & Video::ColorMask::G)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
            }

            if (blendStates.writeMask & Video::ColorMask::B)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
            }

            if (blendStates.writeMask & Video::ColorMask::A)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
            }

            CComPtr<ID3D11BlendState> d3dStates;
            resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
            if (SUCCEEDED(resultValue) && d3dStates)
            {
                resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::IndependentBlendStates &blendStates)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_BLEND_DESC blendDescription;
            blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
            blendDescription.IndependentBlendEnable = true;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                blendDescription.RenderTarget[renderTarget].BlendEnable = blendStates.targetStates[renderTarget].enable;
                blendDescription.RenderTarget[renderTarget].SrcBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorSource)];
                blendDescription.RenderTarget[renderTarget].DestBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorDestination)];
                blendDescription.RenderTarget[renderTarget].BlendOp = DirectX::BlendOperationList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorOperation)];
                blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaSource)];
                blendDescription.RenderTarget[renderTarget].DestBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaDestination)];
                blendDescription.RenderTarget[renderTarget].BlendOpAlpha = DirectX::BlendOperationList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaOperation)];
                blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                if (blendStates.targetStates[renderTarget].writeMask & Video::ColorMask::R)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                }

                if (blendStates.targetStates[renderTarget].writeMask & Video::ColorMask::G)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                }

                if (blendStates.targetStates[renderTarget].writeMask & Video::ColorMask::B)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                }

                if (blendStates.targetStates[renderTarget].writeMask & Video::ColorMask::A)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                }
            }

            CComPtr<ID3D11BlendState> d3dStates;
            resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
            if (SUCCEEDED(resultValue) && d3dStates)
            {
                resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP createSamplerStates(IUnknown **returnObject, const Video::SamplerStates &samplerStates)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_SAMPLER_DESC samplerDescription;
            samplerDescription.AddressU = DirectX::AddressModeList[static_cast<UINT8>(samplerStates.addressModeU)];
            samplerDescription.AddressV = DirectX::AddressModeList[static_cast<UINT8>(samplerStates.addressModeV)];
            samplerDescription.AddressW = DirectX::AddressModeList[static_cast<UINT8>(samplerStates.addressModeW)];
            samplerDescription.MipLODBias = samplerStates.mipLevelBias;
            samplerDescription.MaxAnisotropy = samplerStates.maximumAnisotropy;
            samplerDescription.ComparisonFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(samplerStates.comparisonFunction)];
            samplerDescription.BorderColor[0] = samplerStates.borderColor.r;
            samplerDescription.BorderColor[1] = samplerStates.borderColor.g;
            samplerDescription.BorderColor[2] = samplerStates.borderColor.b;
            samplerDescription.BorderColor[3] = samplerStates.borderColor.a;
            samplerDescription.MinLOD = samplerStates.minimumMipLevel;
            samplerDescription.MaxLOD = samplerStates.maximumMipLevel;
            samplerDescription.Filter = DirectX::FilterList[static_cast<UINT8>(samplerStates.filterMode)];

            CComPtr<ID3D11SamplerState> d3dStates;
            resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates);
            if (SUCCEEDED(resultValue) && d3dStates)
            {
                resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject));
            }

            return resultValue;
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, Video::Format format, UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
            GEK_REQUIRE_RETURN(stride > 0, E_INVALIDARG);
            GEK_REQUIRE_RETURN(count > 0, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            D3D11_BUFFER_DESC bufferDescription;
            bufferDescription.ByteWidth = (stride * count);
            switch (type)
            {
            case Video::BufferType::Structured:
                bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                bufferDescription.StructureByteStride = stride;
                bufferDescription.BindFlags = 0;
                break;

            default:
                bufferDescription.MiscFlags = 0;
                bufferDescription.StructureByteStride = 0;
                switch (type)
                {
                case Video::BufferType::Vertex:
                    bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                    break;

                case Video::BufferType::Index:
                    bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                    break;

                case Video::BufferType::Constant:
                    bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    break;

                default:
                    bufferDescription.BindFlags = 0;
                    break;
                };
            };

            if (data != nullptr)
            {
                bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
                bufferDescription.CPUAccessFlags = 0;
            }
            else if (flags & Video::BufferFlags::Staging)
            {
                bufferDescription.Usage = D3D11_USAGE_STAGING;
                bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            }
            else if (flags & Video::BufferFlags::Mappable)
            {
                bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
                bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
            else
            {
                bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                bufferDescription.CPUAccessFlags = 0;
            }

            if (flags & Video::BufferFlags::Resource)
            {
                bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }

            if (flags & Video::BufferFlags::UnorderedAccess)
            {
                bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            CComPtr<ID3D11Buffer> d3dBuffer;
            if (data == nullptr)
            {
                resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer);
            }
            else
            {
                D3D11_SUBRESOURCE_DATA resourceData;
                resourceData.pSysMem = data;
                resourceData.SysMemPitch = 0;
                resourceData.SysMemSlicePitch = 0;
                resultValue = d3dDevice->CreateBuffer(&bufferDescription, &resourceData, &d3dBuffer);
            }

            if (d3dBuffer)
            {
                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (flags & Video::BufferFlags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<UINT8>(format)];
                    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = count;
                    resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue))
                    {
                        return resultValue;
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (flags & Video::BufferFlags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<UINT8>(format)];
                    viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = count;
                    viewDescription.Buffer.Flags = 0;

                    resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue))
                    {
                        return resultValue;
                    }
                }

                resultValue = E_OUTOFMEMORY;
                CComPtr<BufferImplementation> buffer(new BufferImplementation(format, stride, count, d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView));
                if (buffer)
                {
                    resultValue = buffer->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }

            if (FAILED(resultValue))
            {
                return resultValue;
            }

            return resultValue;
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            return createBuffer(returnObject, Video::Format::Unknown, stride, count, type, flags, data);
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, Video::Format format, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            UINT32 stride = DirectX::FormatStrideList[static_cast<UINT8>(format)];
            return createBuffer(returnObject, format, stride, count, type, flags, data);
        }

        STDMETHODIMP_(void) updateBuffer(VideoBuffer *buffer, LPCVOID data)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(data);

            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);

            d3dDeviceContext->UpdateSubresource(d3dBuffer, 0, nullptr, data, 0, 0);
        }

        STDMETHODIMP mapBuffer(VideoBuffer *buffer, void **data, Video::Map mapping)
        {
            GEK_REQUIRE_RETURN(d3dDeviceContext, E_FAIL);
            GEK_REQUIRE_RETURN(data, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            if (d3dBuffer)
            {
                D3D11_MAP d3dMapping = DirectX::MapList[static_cast<UINT8>(mapping)];

                D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                mappedSubResource.pData = nullptr;
                mappedSubResource.RowPitch = 0;
                mappedSubResource.DepthPitch = 0;
                
                resultValue = d3dDeviceContext->Map(d3dBuffer, 0, d3dMapping, 0, &mappedSubResource);
                if (SUCCEEDED(resultValue))
                {
                    (*data) = mappedSubResource.pData;
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(void) unmapBuffer(VideoBuffer *buffer)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(buffer);

            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->Unmap(d3dBuffer, 0);
        }

        STDMETHODIMP_(void) copyResource(IUnknown *destination, IUnknown *source)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(destination);
            GEK_REQUIRE_VOID_RETURN(source);

            CComQIPtr<ID3D11Resource> d3dDestination(destination);
            CComQIPtr<ID3D11Resource> d3dSource(source);
            d3dDeviceContext->CopyResource(d3dDestination, d3dSource);
        }

        STDMETHODIMP compileComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_FAIL);
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);
            GEK_REQUIRE_RETURN(programScript, E_INVALIDARG);
            GEK_REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            if (defineList != nullptr)
            {
                for (auto &define : (*defineList))
                {
                    D3D_SHADER_MACRO d3dShaderMacro = { define.first.GetString(), define.second.GetString() };
                    d3dShaderMacroList.push_back(d3dShaderMacro);
                }
            }

            d3dShaderMacroList.push_back({ "_COMPUTE_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "cs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            if (SUCCEEDED(resultValue) && d3dShaderBlob)
            {
                CComPtr<ID3D11ComputeShader> d3dShader;
                resultValue = d3dDevice->CreateComputeShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
                if (SUCCEEDED(resultValue) && d3dShader)
                {
                    resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }
            else if (d3dCompilerErrors)
            {
                OutputDebugStringA(LPCSTR(d3dCompilerErrors->GetBufferPointer()));
            }

            return resultValue;
        }

        STDMETHODIMP compileVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> *elementLayout, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_FAIL);
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);
            GEK_REQUIRE_RETURN(programScript, E_INVALIDARG);
            GEK_REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            if (defineList != nullptr)
            {
                for (auto &kPair : (*defineList))
                {
                    D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                    d3dShaderMacroList.push_back(d3dShaderMacro);
                }
            }

            d3dShaderMacroList.push_back({ "_VERTEX_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "vs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            if (SUCCEEDED(resultValue) && d3dShaderBlob)
            {
                CComPtr<ID3D11VertexShader> d3dShader;
                resultValue = d3dDevice->CreateVertexShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
                if (SUCCEEDED(resultValue) && d3dShader)
                {
                    CComPtr<ID3D11InputLayout> d3dInputLayout;
                    if (elementLayout && !(*elementLayout).empty())
                    {
                        Video::ElementType lastElementType = Video::ElementType::Vertex;
                        std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementList;
                        for (auto &element : (*elementLayout))
                        {
                            D3D11_INPUT_ELEMENT_DESC elementDesc;
                            if (lastElementType != element.slotClass)
                            {
                                elementDesc.AlignedByteOffset = 0;
                            }
                            else
                            {
                                elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                            }

                            lastElementType = element.slotClass;
                            elementDesc.SemanticName = element.semanticName;
                            elementDesc.SemanticIndex = element.semanticIndex;
                            elementDesc.InputSlot = element.slotIndex;
                            switch (element.slotClass)
                            {
                            case Video::ElementType::Instance:
                                elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                                elementDesc.InstanceDataStepRate = 1;
                                break;

                            case Video::ElementType::Vertex:
                            default:
                                elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                                elementDesc.InstanceDataStepRate = 0;
                            };

                            elementDesc.Format = DirectX::BufferFormatList[static_cast<UINT8>(element.format)];
                            if (elementDesc.Format == DXGI_FORMAT_UNKNOWN)
                            {
                                break;
                            }

                            inputElementList.push_back(elementDesc);
                        }

                        if (inputElementList.size() == (*elementLayout).size())
                        {
                            resultValue = d3dDevice->CreateInputLayout(inputElementList.data(), inputElementList.size(), d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), &d3dInputLayout);
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<VertexProgram> shader(new VertexProgram(d3dShader, d3dInputLayout));
                        if (shader)
                        {
                            resultValue = shader->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }
            }
            else if (d3dCompilerErrors)
            {
                OutputDebugStringA(LPCSTR(d3dCompilerErrors->GetBufferPointer()));
            }

            return resultValue;
        }

        STDMETHODIMP compileGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_FAIL);
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);
            GEK_REQUIRE_RETURN(programScript, E_INVALIDARG);
            GEK_REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            if (defineList != nullptr)
            {
                for (auto &kPair : (*defineList))
                {
                    D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                    d3dShaderMacroList.push_back(d3dShaderMacro);
                }
            }

            d3dShaderMacroList.push_back({ "_GEOMETRY_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "gs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            if (SUCCEEDED(resultValue) && d3dShaderBlob)
            {
                CComPtr<ID3D11GeometryShader> d3dShader;
                resultValue = d3dDevice->CreateGeometryShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
                if (SUCCEEDED(resultValue) && d3dShader)
                {
                    resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }
            else if (d3dCompilerErrors)
            {
                OutputDebugStringA(LPCSTR(d3dCompilerErrors->GetBufferPointer()));
            }

            return resultValue;
        }

        STDMETHODIMP compilePixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_FAIL);
            GEK_REQUIRE_RETURN(returnObject, E_INVALIDARG);
            GEK_REQUIRE_RETURN(programScript, E_INVALIDARG);
            GEK_REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            if (defineList != nullptr)
            {
                for (auto &kPair : (*defineList))
                {
                    D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                    d3dShaderMacroList.push_back(d3dShaderMacro);
                }
            }

            d3dShaderMacroList.push_back({ "_PIXEL_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "ps_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            if (SUCCEEDED(resultValue) && d3dShaderBlob)
            {
                CComPtr<ID3D11PixelShader> d3dShader;
                resultValue = d3dDevice->CreatePixelShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
                if (SUCCEEDED(resultValue) && d3dShader)
                {
                    resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject));
                }
            }
            else if (d3dCompilerErrors)
            {
                OutputDebugStringA(LPCSTR(d3dCompilerErrors->GetBufferPointer()));
            }

            return resultValue;
        }

        STDMETHODIMP compileComputeProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            CComPtr<IncludeImplementation> include(new IncludeImplementation(L"", onInclude));
            return compileComputeProgram(returnObject, nullptr, programScript, entryFunction, defineList, include);
        }

        STDMETHODIMP compileVertexProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> *elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            CComPtr<IncludeImplementation> include(new IncludeImplementation(L"", onInclude));
            return compileVertexProgram(returnObject, nullptr, programScript, entryFunction, elementLayout, defineList, include);
        }

        STDMETHODIMP compileGeometryProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            CComPtr<IncludeImplementation> include(new IncludeImplementation(L"", onInclude));
            return compileGeometryProgram(returnObject, nullptr, programScript, entryFunction, defineList, include);
        }

        STDMETHODIMP compilePixelProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            CComPtr<IncludeImplementation> include(new IncludeImplementation(L"", onInclude));
            return compilePixelProgram(returnObject, nullptr, programScript, entryFunction, defineList, include);
        }

        STDMETHODIMP loadComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            CStringA progamScript;
            resultValue = Gek::FileSystem::load(fileName, progamScript);
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileComputeProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Video::InputElement> *elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            CStringA progamScript;
            resultValue = Gek::FileSystem::load(fileName, progamScript);
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileVertexProgram(returnObject, fileName, progamScript, entryFunction, elementLayout, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            CStringA progamScript;
            resultValue = Gek::FileSystem::load(fileName, progamScript);
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileGeometryProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadPixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            CStringA progamScript;
            resultValue = Gek::FileSystem::load(fileName, progamScript);
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compilePixelProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP createTexture(VideoTexture **returnObject, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, UINT32 flags, UINT32 mipmaps)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            UINT32 bindFlags = 0;
            if (flags & Video::TextureFlags::RenderTarget)
            {
                if (flags & Video::TextureFlags::DepthTarget)
                {
                    return E_INVALIDARG;
                }

                bindFlags |= D3D11_BIND_RENDER_TARGET;
            }

            if (flags & Video::TextureFlags::DepthTarget)
            {
                if (flags & Video::TextureFlags::RenderTarget)
                {
                    return E_INVALIDARG;
                }

                if (depth > 1)
                {
                    return E_INVALIDARG;
                }

                bindFlags |= D3D11_BIND_DEPTH_STENCIL;
            }

            if (flags & Video::TextureFlags::Resource)
            {
                bindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }

            if (flags & Video::TextureFlags::UnorderedAccess)
            {
                bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            }

            DXGI_FORMAT d3dFormat = DirectX::TextureFormatList[static_cast<UINT8>(format)];;
            CComQIPtr<ID3D11Resource> d3dResource;
            if (depth == 1)
            {
                D3D11_TEXTURE2D_DESC textureDescription;
                textureDescription.Width = width;
                textureDescription.Height = height;
                textureDescription.MipLevels = mipmaps;
                textureDescription.Format = d3dFormat;
                textureDescription.ArraySize = 1;
                textureDescription.SampleDesc.Count = 1;
                textureDescription.SampleDesc.Quality = 0;
                textureDescription.Usage = D3D11_USAGE_DEFAULT;
                textureDescription.BindFlags = bindFlags;
                textureDescription.CPUAccessFlags = 0;
                textureDescription.MiscFlags = (mipmaps == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);

                CComPtr<ID3D11Texture2D> texture2D;
                resultValue = d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture2D);
                if (SUCCEEDED(resultValue) && texture2D)
                {
                    d3dResource = texture2D;
                }
            }
            else
            {
                D3D11_TEXTURE3D_DESC textureDescription;
                textureDescription.Width = width;
                textureDescription.Height = height;
                textureDescription.Depth = depth;
                textureDescription.MipLevels = mipmaps;
                textureDescription.Format = d3dFormat;
                textureDescription.Usage = D3D11_USAGE_DEFAULT;
                textureDescription.BindFlags = bindFlags;
                textureDescription.CPUAccessFlags = 0;
                textureDescription.MiscFlags = (mipmaps == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);

                CComPtr<ID3D11Texture3D> texture2D;
                resultValue = d3dDevice->CreateTexture3D(&textureDescription, nullptr, &texture2D);
                if (SUCCEEDED(resultValue) && texture2D)
                {
                    d3dResource = texture2D;
                }
            }

            if (d3dResource)
            {
                CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                if (SUCCEEDED(resultValue) && flags & Video::TextureFlags::RenderTarget)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                    renderViewDescription.Format = d3dFormat;
                    if (depth == 1)
                    {
                        renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        renderViewDescription.Texture2D.MipSlice = 0;
                    }
                    else
                    {
                        renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                        renderViewDescription.Texture3D.MipSlice = 0;
                        renderViewDescription.Texture3D.FirstWSlice = 0;
                        renderViewDescription.Texture3D.WSize = depth;
                    }

                    resultValue = d3dDevice->CreateRenderTargetView(d3dResource, &renderViewDescription, &d3dRenderTargetView);
                }

                CComPtr<ID3D11DepthStencilView> depthStencilView;
                if (SUCCEEDED(resultValue) && flags & Video::TextureFlags::DepthTarget)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = d3dFormat;
                    depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &depthStencilView);
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (SUCCEEDED(resultValue) && flags & Video::TextureFlags::Resource)
                {
                    resultValue = d3dDevice->CreateShaderResourceView(d3dResource, nullptr, &d3dShaderResourceView);
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (flags & Video::TextureFlags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = d3dFormat;
                    if (depth == 1)
                    {
                        viewDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                        viewDescription.Texture2D.MipSlice = 0;
                    }
                    else
                    {
                        viewDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                        viewDescription.Texture3D.MipSlice = 0;
                        viewDescription.Texture3D.FirstWSlice = 0;
                        viewDescription.Texture3D.WSize = depth;
                    }

                    resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView);
                }

                if (SUCCEEDED(resultValue))
                {
                    CComPtr<IUnknown> texture;
                    if (flags & Video::TextureFlags::RenderTarget)
                    {
                        texture = new TextureImplementation(d3dResource, d3dRenderTargetView, d3dShaderResourceView, d3dUnorderedAccessView, format, width, height);
                    }
                    else if (flags & Video::TextureFlags::DepthTarget)
                    {
                        texture = new TextureImplementation(d3dResource, depthStencilView, d3dShaderResourceView, d3dUnorderedAccessView, format, width, height);
                    }
                    else
                    {
                        texture = new TextureImplementation(d3dResource, d3dShaderResourceView, d3dUnorderedAccessView, format, width, height, depth);
                    }

                    if (texture)
                    {
                        resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadTexture(VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
            GEK_REQUIRE_RETURN(fileName, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            std::vector<UINT8> fileData;
            resultValue = Gek::FileSystem::load(fileName, fileData);
            if (SUCCEEDED(resultValue))
            {
                ::DirectX::ScratchImage scratchImage;
                ::DirectX::TexMetadata textureMetaData;

                CPathW filePath(fileName);
                CStringW extension(filePath.GetExtension());
                if (extension.CompareNoCase(L".dds") == 0)
                {
                    resultValue = ::DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, &textureMetaData, scratchImage);
                }
                else if(extension.CompareNoCase(L".tga") == 0)
                {
                    resultValue = ::DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), &textureMetaData, scratchImage);
                }
                else if (extension.CompareNoCase(L".png") == 0)
                {
                    resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_PNG, &textureMetaData, scratchImage);
                }
                else if (extension.CompareNoCase(L".bmp") == 0)
                {
                    resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_BMP, &textureMetaData, scratchImage);
                }
                else if (extension.CompareNoCase(L".jpg") == 0 ||
                         extension.CompareNoCase(L".jpeg") == 0)
                {
                    resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_JPEG, &textureMetaData, scratchImage);
                }

                if (SUCCEEDED(resultValue))
                {
                    if (flags && Video::TextureLoadFlags::sRGB)
                    {
                        switch (scratchImage.GetMetadata().format)
                        {
                        case DXGI_FORMAT_R8G8B8A8_UNORM:
                            scratchImage.OverrideFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
                            break;

                        case DXGI_FORMAT_BC1_UNORM:
                            scratchImage.OverrideFormat(DXGI_FORMAT_BC1_UNORM_SRGB);
                            break;

                        case DXGI_FORMAT_BC2_UNORM:
                            scratchImage.OverrideFormat(DXGI_FORMAT_BC2_UNORM_SRGB);
                            break;

                        case DXGI_FORMAT_BC3_UNORM:
                            scratchImage.OverrideFormat(DXGI_FORMAT_BC3_UNORM_SRGB);
                            break;

                        case DXGI_FORMAT_BC7_UNORM:
                            scratchImage.OverrideFormat(DXGI_FORMAT_BC7_UNORM_SRGB);
                            break;
                        };
                    }

                    CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                    if (true)
                    {
                        resultValue = ::DirectX::CreateShaderResourceView(d3dDevice, scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), &d3dShaderResourceView);
                    }

                    if (SUCCEEDED(resultValue) && d3dShaderResourceView)
                    {
                        resultValue = E_UNEXPECTED;
                        CComPtr<ID3D11Resource> d3dResource;
                        d3dShaderResourceView->GetResource(&d3dResource);
                        if (d3dResource)
                        {
                            resultValue = E_OUTOFMEMORY;
                            CComPtr<TextureImplementation> texture(new TextureImplementation(d3dResource, d3dShaderResourceView, nullptr, Video::Format::Unknown, scratchImage.GetMetadata().width, scratchImage.GetMetadata().height, scratchImage.GetMetadata().depth));
                            if (texture)
                            {
                                resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                            }
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadCubeMap(VideoTexture **returnObject, LPCWSTR fileNameList[6], UINT32 flags)
        {
            GEK_REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;

            ::DirectX::ScratchImage cubeMapList[6];
            ::DirectX::TexMetadata cubeMapMetaData;
            for (UINT32 side = 0; side < 6; side++)
            {
                std::vector<UINT8> fileData;
                resultValue = Gek::FileSystem::load(fileNameList[side], fileData);
                if (SUCCEEDED(resultValue))
                {
                    CPathW filePath(fileNameList[side]);
                    CStringW extension(filePath.GetExtension());
                    if (extension.CompareNoCase(L".dds") == 0)
                    {
                        resultValue = ::DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, &cubeMapMetaData, cubeMapList[side]);
                    }
                    else if (extension.CompareNoCase(L".tga") == 0)
                    {
                        resultValue = ::DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), &cubeMapMetaData, cubeMapList[side]);
                    }
                    else if (extension.CompareNoCase(L".png") == 0)
                    {
                        resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_PNG, &cubeMapMetaData, cubeMapList[side]);
                    }
                    else if (extension.CompareNoCase(L".bmp") == 0)
                    {
                        resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_BMP, &cubeMapMetaData, cubeMapList[side]);
                    }
                    else if (extension.CompareNoCase(L".jpg") == 0 ||
                        extension.CompareNoCase(L".jpeg") == 0)
                    {
                        resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_JPEG, &cubeMapMetaData, cubeMapList[side]);
                    }
                }

                if (FAILED(resultValue))
                {
                    break;
                }
            }

            if (SUCCEEDED(resultValue))
            {
                ::DirectX::Image imageList[6] =
                {
                    *cubeMapList[0].GetImage(0, 0, 0),
                    *cubeMapList[1].GetImage(0, 0, 0),
                    *cubeMapList[2].GetImage(0, 0, 0),
                    *cubeMapList[3].GetImage(0, 0, 0),
                    *cubeMapList[4].GetImage(0, 0, 0),
                    *cubeMapList[5].GetImage(0, 0, 0),
                };

                ::DirectX::ScratchImage cubeMap;
                cubeMap.InitializeCubeFromImages(imageList, 6, 0);
                if (flags && Video::TextureLoadFlags::sRGB)
                {
                    switch (cubeMap.GetMetadata().format)
                    {
                    case DXGI_FORMAT_R8G8B8A8_UNORM:
                        cubeMap.OverrideFormat(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
                        break;

                    case DXGI_FORMAT_BC1_UNORM:
                        cubeMap.OverrideFormat(DXGI_FORMAT_BC1_UNORM_SRGB);
                        break;

                    case DXGI_FORMAT_BC2_UNORM:
                        cubeMap.OverrideFormat(DXGI_FORMAT_BC2_UNORM_SRGB);
                        break;

                    case DXGI_FORMAT_BC3_UNORM:
                        cubeMap.OverrideFormat(DXGI_FORMAT_BC3_UNORM_SRGB);
                        break;

                    case DXGI_FORMAT_BC7_UNORM:
                        cubeMap.OverrideFormat(DXGI_FORMAT_BC7_UNORM_SRGB);
                        break;
                    };
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                resultValue = ::DirectX::CreateShaderResourceView(d3dDevice, cubeMap.GetImages(), cubeMap.GetImageCount(), cubeMap.GetMetadata(), &d3dShaderResourceView);
                if (SUCCEEDED(resultValue) && d3dShaderResourceView)
                {
                    resultValue = E_UNEXPECTED;
                    CComPtr<ID3D11Resource> d3dResource;
                    d3dShaderResourceView->GetResource(&d3dResource);
                    if (d3dResource)
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<TextureImplementation> texture(new TextureImplementation(d3dResource, d3dShaderResourceView, nullptr, Video::Format::Unknown, cubeMapMetaData.width, cubeMapMetaData.height, 1));
                        if (texture)
                        {
                            resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject));
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(void) updateTexture(VideoTexture *texture, LPCVOID data, UINT32 pitch, Shapes::Rectangle<UINT32> *destinationRectangle)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);

            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(texture);
            if (d3dShaderResourceView)
            {
                CComQIPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                if (d3dResource)
                {
                    if (destinationRectangle == nullptr)
                    {
                        d3dDeviceContext->UpdateSubresource(d3dResource, 0, nullptr, data, pitch, pitch);
                    }
                    else
                    {
                        D3D11_BOX destinationBox =
                        {
                            destinationRectangle->left,
                            destinationRectangle->top,
                            0,
                            destinationRectangle->right,
                            destinationRectangle->bottom,
                            1,
                        };

                        d3dDeviceContext->UpdateSubresource(d3dResource, 0, &destinationBox, data, pitch, pitch);
                    }
                }
            }
        }

        STDMETHODIMP_(void) executeCommandList(IUnknown *commandList)
        {
            GEK_REQUIRE_VOID_RETURN(d3dDeviceContext);
            GEK_REQUIRE_VOID_RETURN(commandList);

            CComQIPtr<ID3D11CommandList> d3dCommandList(commandList);
            d3dDeviceContext->ExecuteCommandList(d3dCommandList, FALSE);
        }

        STDMETHODIMP_(void) present(bool waitForVerticalSync)
        {
            GEK_REQUIRE_VOID_RETURN(dxSwapChain);

            dxSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
        }
    };

    REGISTER_CLASS(VideoSystemImplementation);
}; // namespace Gek
