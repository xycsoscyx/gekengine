#pragma warning(disable : 4005)

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\Plugin.h"
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

    class CommandList
        : public VideoObject
    {
    public:
        CComPtr<ID3D11CommandList> d3dCommandList;

    public:
        CommandList(ID3D11CommandList *d3dCommandList)
            : d3dCommandList(d3dCommandList)
        {
        }
    };

    class RenderState
        : public VideoObject
    {
    public:
        CComQIPtr<ID3D11RasterizerState> d3dRenderState;

    public:
        RenderState(ID3D11RasterizerState *d3dRenderState)
            : d3dRenderState(d3dRenderState)
        {
        }
    };

    class DepthState
        : public VideoObject
    {
    public:
        CComQIPtr<ID3D11DepthStencilState> d3dDepthState;

    public:
        DepthState(ID3D11DepthStencilState *d3dDepthState)
            : d3dDepthState(d3dDepthState)
        {
        }
    };

    class BlendState
        : public VideoObject
    {
    public:
        CComQIPtr<ID3D11BlendState> d3dBlendState;

    public:
        BlendState(ID3D11BlendState *d3dBlendState)
            : d3dBlendState(d3dBlendState)
        {
        }
    };

    class SamplerState
        : public VideoObject
    {
    public:
        CComQIPtr<ID3D11SamplerState> d3dSamplerState;

    public:
        SamplerState(ID3D11SamplerState *d3dSamplerState)
            : d3dSamplerState(d3dSamplerState)
        {
        }
    };

    class Event
        : public VideoObject
    {
    public:
        CComPtr<ID3D11Query> d3dQuery;

    public:
        Event(ID3D11Query *d3dQuery)
            : d3dQuery(d3dQuery)
        {
        }
    };

    class ComputeProgram
        : public VideoObject
    {
    public:
        CComPtr<ID3D11ComputeShader> d3dShader;

    public:
        ComputeProgram(ID3D11ComputeShader *d3dShader)
            : d3dShader(d3dShader)
        {
        }
    };

    class VertexProgram
        : public VideoObject
    {
    public:
        CComPtr<ID3D11VertexShader> d3dShader;
        CComPtr<ID3D11InputLayout> d3dInputLayout;

    public:
        VertexProgram(ID3D11VertexShader *d3dShader, ID3D11InputLayout *d3dInputLayout)
            : d3dShader(d3dShader)
            , d3dInputLayout(d3dInputLayout)
        {
        }
    };

    class GeometryProgram
        : public VideoObject
    {
    public:
        CComPtr<ID3D11GeometryShader> d3dShader;

    public:
        GeometryProgram(ID3D11GeometryShader *d3dShader)
            : d3dShader(d3dShader)
        {
        }
    };

    class PixelProgram
        : public VideoObject
    {
    public:
        CComPtr<ID3D11PixelShader> d3dShader;

    public:
        PixelProgram(ID3D11PixelShader *d3dShader)
            : d3dShader(d3dShader)
        {
        }
    };

    class Resource
        : virtual public VideoObject
    {
    public:
        CComQIPtr<ID3D11Resource> d3dResource;

    public:
        template <typename TYPE>
        Resource(TYPE *d3dResource)
            : d3dResource(d3dResource)
        {
        }
    };

    class ResourceView
        : virtual public VideoObject
    {
    public:
        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;

    public:
        ResourceView(ID3D11ShaderResourceView *d3dShaderResourceView)
            : d3dShaderResourceView(d3dShaderResourceView)
        {
        }
    };

    class UnorderedAccessView
        : virtual public VideoObject
    {
    public:
        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;

    public:
        UnorderedAccessView(ID3D11UnorderedAccessView *d3dUnorderedAccessView)
            : d3dUnorderedAccessView(d3dUnorderedAccessView)
        {
        }
    };

    class BaseBuffer
        : virtual public VideoBuffer
        , public Resource
    {
    public:
        CComPtr<ID3D11Buffer> d3dBuffer;
        Video::Format format;
        UINT32 stride;
        UINT32 count;

    public:
        BaseBuffer(ID3D11Buffer *d3dBuffer, Video::Format format, UINT32 stride, UINT32 count)
            : Resource(d3dBuffer)
            , d3dBuffer(d3dBuffer)
            , format(format)
            , stride(stride)
            , count(count)
        {
        }

        // VideoBuffer
        Video::Format getFormat(void)
        {
            return format;
        }

        UINT32 getStride(void)
        {
            return stride;
        }

        UINT32 getCount(void)
        {
            return stride;
        }
    };

    class ViewBuffer
        : public BaseBuffer
        , public ResourceView
        , public UnorderedAccessView
    {
    public:
        ViewBuffer(ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 stride, UINT32 count)
            : BaseBuffer(d3dBuffer, format, stride, count)
            , ResourceView(d3dShaderResourceView)
            , UnorderedAccessView(d3dUnorderedAccessView)
        {
        }
    };

    class BaseTexture
        : public Resource
        , virtual public VideoTexture
    {
    public:
        Video::Format format;
        UINT32 width;
        UINT32 height;
        UINT32 depth;
        Video::ViewPort viewPort;

    public:
        BaseTexture(ID3D11Resource *d3dResource, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : Resource(d3dResource)
            , format(format)
            , width(width)
            , height(height)
            , depth(depth)
        {
        }

        // VideoTexture
        Video::Format getFormat(void)
        {
            return format;
        }

        UINT32 getWidth(void)
        {
            return width;
        }

        UINT32 getHeight(void)
        {
            return height;
        }

        UINT32 getDepth(void)
        {
            return depth;
        }
    };

    class ViewTexture
        : public BaseTexture
        , public ResourceView
        , public UnorderedAccessView
    {
    public:
        ViewTexture(ID3D11Resource *d3dResource, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : BaseTexture(d3dResource, format, width, height, depth)
            , ResourceView(d3dShaderResourceView)
            , UnorderedAccessView(d3dUnorderedAccessView)
        {
        }
    };

    class TargetTexture
        : public BaseTexture
        , virtual public VideoTarget
    {
    public:
        CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
        Video::ViewPort viewPort;

    public:
        TargetTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : BaseTexture(d3dResource, format, width, height, depth)
            , d3dRenderTargetView(d3dRenderTargetView)
            , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(width), float(height)), 0.0f, 1.0f)
        {
        }

        // VideoTarget
        const Video::ViewPort &getViewPort(void)
        {
            return viewPort;
        }
    };

    class TargetViewTexture
        : public TargetTexture
        , public ResourceView
        , public UnorderedAccessView
    {
    public:
        CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
        Video::ViewPort viewPort;

    public:
        TargetViewTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : TargetTexture(d3dResource, d3dRenderTargetView, format, width, height, depth)
            , ResourceView(d3dShaderResourceView)
            , UnorderedAccessView(d3dUnorderedAccessView)
        {
        }
    };

    class DepthTexture
        : public BaseTexture
        , public ResourceView
        , public UnorderedAccessView
    {
    public:
        CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;

    public:
        DepthTexture(ID3D11Resource *d3dResource, ID3D11DepthStencilView *d3dDepthStencilView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : BaseTexture(d3dResource, format, width, height, depth)
            , ResourceView(d3dShaderResourceView)
            , UnorderedAccessView(d3dUnorderedAccessView)
            , d3dDepthStencilView(d3dDepthStencilView)
        {
        }
    };

    class IncludeImplementation
        : public ID3DInclude
    {
    public:
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

        // ID3DInclude
        STDMETHODIMP Open(D3D_INCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize)
        {
            GEK_REQUIRE(fileName);
            GEK_REQUIRE(data);
            GEK_REQUIRE(dataSize);

            includeBuffer.clear();
            try
            {
                FileSystem::load(CA2W(fileName), includeBuffer);
            }
            catch (FileSystem::Exception)
            {
                try
                {
                    CPathW shaderPath;
                    shaderPath.Combine(shaderFilePath, CA2W(fileName));
                    FileSystem::load(shaderPath, includeBuffer);
                }
                catch (FileSystem::Exception)
                {
                    try
                    {
                        onInclude(fileName, includeBuffer);
                    }
                    catch (FileSystem::Exception)
                    {
                    };
                };
            };

            if (!includeBuffer.empty())
            {
                (*data) = includeBuffer.data();
                (*dataSize) = includeBuffer.size();
                return S_OK;
            }

            return E_FAIL;
        }

        STDMETHODIMP Close(LPCVOID data)
        {
            return S_OK;
        }
    };

    class PipelineImplementation
    {
    protected:
        ID3D11DeviceContext *d3dDeviceContext;

    public:
        PipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : d3dDeviceContext(d3dDeviceContext)
        {
        }
    };

    class ComputePipelineImplementation
        : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        ComputePipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        // VideoPipeline
        void setProgram(VideoObject *program)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->CSSetShader(program ? reinterpret_cast<ComputeProgram *>(program)->d3dShader : nullptr, nullptr, 0);
        }

        void setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->CSSetConstantBuffers(stage, 1, &(buffer ? reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer : nullptr));
        }

        void setSamplerState(VideoObject *samplerState, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->CSSetSamplers(stage, 1, &(samplerState ? reinterpret_cast<SamplerState *>(samplerState)->d3dSamplerState : nullptr));
        }

        void setResource(VideoObject *resource, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->CSSetShaderResources(stage, 1, &(resource ? reinterpret_cast<ResourceView *>(resource)->d3dShaderResourceView : nullptr));
        }

        void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->CSSetUnorderedAccessViews(stage, 1, &(unorderedAccess ? reinterpret_cast<UnorderedAccessView *>(unorderedAccess)->d3dUnorderedAccessView : nullptr), nullptr);
        }
    };

    class VertexPipelineImplementation
        : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        VertexPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        // VideoPipeline
        void setProgram(VideoObject *program)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->VSSetShader(program ? reinterpret_cast<VertexProgram *>(program)->d3dShader : nullptr, nullptr, 0);
            d3dDeviceContext->IASetInputLayout(program ? reinterpret_cast<VertexProgram *>(program)->d3dInputLayout : nullptr);
        }

        void setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->VSSetConstantBuffers(stage, 1, &(buffer ? reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer : nullptr));
        }

        void setSamplerState(VideoObject *samplerState, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->VSSetSamplers(stage, 1, &(samplerState ? reinterpret_cast<SamplerState *>(samplerState)->d3dSamplerState : nullptr));
        }

        void setResource(VideoObject *resource, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->VSSetShaderResources(stage, 1, &(resource ? reinterpret_cast<ResourceView *>(resource)->d3dShaderResourceView : nullptr));
        }

        void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage)
        {
            GEK_THROW_EXCEPTION(Video::Exception, "Unable to set vertex shader unordered access")
        }
    };

    class GeometryPipelineImplementation
        : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        GeometryPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        // VideoPipeline
        void setProgram(VideoObject *program)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->GSSetShader(program ? reinterpret_cast<GeometryProgram *>(program)->d3dShader : nullptr, nullptr, 0);
        }

        void setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->GSSetConstantBuffers(stage, 1, &(buffer ? reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer : nullptr));
        }

        void setSamplerState(VideoObject *samplerState, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->GSSetSamplers(stage, 1, &(samplerState ? reinterpret_cast<SamplerState *>(samplerState)->d3dSamplerState : nullptr));
        }

        void setResource(VideoObject *resource, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->GSSetShaderResources(stage, 1, &(resource ? reinterpret_cast<ResourceView *>(resource)->d3dShaderResourceView : nullptr));
        }

        void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage)
        {
            GEK_THROW_EXCEPTION(Video::Exception, "Unable to set geometry shader unordered access")
        }
    };

    class PixelPipelineImplementation
        : public PipelineImplementation
        , public VideoPipeline
    {
    public:
        PixelPipelineImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : PipelineImplementation(d3dDeviceContext)
        {
        }

        // VideoPipeline
        void setProgram(VideoObject *program)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->PSSetShader(program ? reinterpret_cast<PixelProgram *>(program)->d3dShader : nullptr, nullptr, 0);
        }

        void setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->PSSetConstantBuffers(stage, 1, &(buffer ? reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer : nullptr));
        }

        void setSamplerState(VideoObject *samplerState, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->PSSetSamplers(stage, 1, &(samplerState ? reinterpret_cast<SamplerState *>(samplerState)->d3dSamplerState : nullptr));
        }

        void setResource(VideoObject *resource, UINT32 stage)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->PSSetShaderResources(stage, 1, &(resource ? reinterpret_cast<ResourceView *>(resource)->d3dShaderResourceView : nullptr));
        }

        void setUnorderedAccess(VideoObject *unorderedAccess, UINT32 stage)
        {
            GEK_THROW_EXCEPTION(Video::Exception, "Unable to set pixel shader unordered access")
        }
    };

    class VideoContextImplementation
        : public VideoContext
    {
    public:
        CComPtr<ID3D11DeviceContext> d3dDeviceContext;
        std::shared_ptr<VideoPipeline> computeSystemHandler;
        std::shared_ptr<VideoPipeline> vertexSystemHandler;
        std::shared_ptr<VideoPipeline> geomtrySystemHandler;
        std::shared_ptr<VideoPipeline> pixelSystemHandler;

    public:
        VideoContextImplementation(ID3D11DeviceContext *d3dDeviceContext)
            : d3dDeviceContext(d3dDeviceContext)
            , computeSystemHandler(new ComputePipelineImplementation(d3dDeviceContext))
            , vertexSystemHandler(new VertexPipelineImplementation(d3dDeviceContext))
            , geomtrySystemHandler(new GeometryPipelineImplementation(d3dDeviceContext))
            , pixelSystemHandler(new PixelPipelineImplementation(d3dDeviceContext))
        {
        }

        // VideoContext
        VideoPipeline *computePipeline(void)
        {
            GEK_REQUIRE(computeSystemHandler);

            return computeSystemHandler.get();
        }

        VideoPipeline *vertexPipeline(void)
        {
            GEK_REQUIRE(vertexSystemHandler);

            return vertexSystemHandler.get();
        }

        VideoPipeline *geometryPipeline(void)
        {
            GEK_REQUIRE(geomtrySystemHandler);

            return geomtrySystemHandler.get();
        }

        VideoPipeline *pixelPipeline(void)
        {
            GEK_REQUIRE(pixelSystemHandler);

            return pixelSystemHandler.get();
        }

        void generateMipMaps(VideoTexture *texture)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(texture);

            d3dDeviceContext->GenerateMips(reinterpret_cast<ResourceView *>(texture)->d3dShaderResourceView);
        }

        void clearResources(void)
        {
            GEK_REQUIRE(d3dDeviceContext);

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

        void setViewports(Video::ViewPort *viewPortList, UINT32 viewPortCount)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(viewPortList);
            GEK_REQUIRE(viewPortCount > 0);

            d3dDeviceContext->RSSetViewports(viewPortCount, (D3D11_VIEWPORT *)viewPortList);
        }

        void setScissorRect(Shapes::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(rectangleList);
            GEK_REQUIRE(rectangleCount > 0);

            d3dDeviceContext->RSSetScissorRects(rectangleCount, (D3D11_RECT *)rectangleList);
        }

        void clearRenderTarget(VideoTarget *renderTarget, const Math::Color &colorClear)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(renderTarget);

            d3dDeviceContext->ClearRenderTargetView(reinterpret_cast<TargetTexture *>(renderTarget)->d3dRenderTargetView, colorClear.data);
        }

        void clearDepthStencilTarget(VideoObject *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(depthBuffer);

            d3dDeviceContext->ClearDepthStencilView(reinterpret_cast<DepthTexture *>(depthBuffer)->d3dDepthStencilView,
                ((flags & Video::ClearMask::Depth ? D3D11_CLEAR_DEPTH : 0) |
                 (flags & Video::ClearMask::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                  depthClear, stencilClear);
        }

        ID3D11RenderTargetView *d3dRenderTargetViewList[8];
        void setRenderTargets(VideoTarget **renderTargetList, UINT32 renderTargetCount, VideoObject *depthBuffer)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(renderTargetList);

            for (UINT32 renderTarget = 0; renderTarget < renderTargetCount; renderTarget++)
            {
                d3dRenderTargetViewList[renderTarget] = reinterpret_cast<TargetTexture *>(renderTargetList[renderTarget])->d3dRenderTargetView;
            }

            d3dDeviceContext->OMSetRenderTargets(renderTargetCount, d3dRenderTargetViewList, (depthBuffer ? reinterpret_cast<DepthTexture *>(depthBuffer)->d3dDepthStencilView : nullptr));
        }

        void setRenderState(VideoObject *renderState)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(renderState);

            d3dDeviceContext->RSSetState(reinterpret_cast<RenderState *>(renderState)->d3dRenderState);
        }

        void setDepthState(VideoObject *depthState, UINT32 stencilReference)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(depthState);

            d3dDeviceContext->OMSetDepthStencilState(reinterpret_cast<DepthState *>(depthState)->d3dDepthState, stencilReference);
        }

        void setBlendState(VideoObject *blendState, const Math::Color &blendFactor, UINT32 mask)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(blendState);

            d3dDeviceContext->OMSetBlendState(reinterpret_cast<BlendState *>(blendState)->d3dBlendState, blendFactor.data, mask);
        }

        void setVertexBuffer(UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(vertexBuffer);

            UINT32 stride = vertexBuffer->getStride();
            d3dDeviceContext->IASetVertexBuffers(slot, 1, &reinterpret_cast<BaseBuffer *>(vertexBuffer)->d3dBuffer, &stride, &offset);
        }

        void setIndexBuffer(VideoBuffer *indexBuffer, UINT32 offset)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(indexBuffer);

            DXGI_FORMAT format = (indexBuffer->getFormat() == Video::Format::Short ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
            d3dDeviceContext->IASetIndexBuffer(reinterpret_cast<BaseBuffer *>(indexBuffer)->d3dBuffer, format, offset);
        }

        void setPrimitiveType(Video::PrimitiveType primitiveType)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->IASetPrimitiveTopology(DirectX::TopologList[static_cast<UINT8>(primitiveType)]);
        }

        void drawPrimitive(UINT32 vertexCount, UINT32 firstVertex)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->Draw(vertexCount, firstVertex);
        }

        void drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        void drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
        }

        void drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
        }

        void dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ)
        {
            GEK_REQUIRE(d3dDeviceContext);

            d3dDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        std::shared_ptr<VideoObject> finishCommandList(void)
        {
            GEK_REQUIRE(d3dDeviceContext);

            CComPtr<ID3D11CommandList> d3dCommandList;
            HRESULT resultValue = d3dDeviceContext->FinishCommandList(FALSE, &d3dCommandList);
            GEK_CHECK_EXCEPTION(!d3dCommandList, Video::Exception, "Unable to finish command list: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<CommandList>(d3dCommandList.p));
        }
    };

    class VideoSystemImplementation
        : public Plugin<VideoSystemImplementation, HWND, bool, Video::Format>
        , public VideoSystem
    {
    public:
        HWND window;
        bool isChildWindow;
        bool fullScreen;
        Video::Format format;

        CComPtr<ID3D11Device> d3dDevice;
        CComPtr<ID3D11DeviceContext> d3dDeviceContext;
        CComPtr<IDXGISwapChain> dxSwapChain;

        std::shared_ptr<VideoContext> defaultContext;
        std::shared_ptr<VideoTarget> backBuffer;

    public:
        VideoSystemImplementation(Context *context, HWND window, bool fullScreen, Video::Format format)
            : Plugin(context)
            , window(window)
            , isChildWindow(GetParent(window) != nullptr)
            , fullScreen(fullScreen)
            , format(format)
        {
            GEK_REQUIRE(window);

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
            HRESULT resultValue = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, _ARRAYSIZE(featureLevelList), D3D11_SDK_VERSION, &swapChainDescription, &dxSwapChain, &d3dDevice, &featureLevel, &d3dDeviceContext);
            GEK_CHECK_EXCEPTION(featureLevel != featureLevelList[0], Video::Exception, "Required Direect3D feature level not supported: %d", resultValue);
            GEK_CHECK_EXCEPTION(!dxSwapChain, Video::Exception, "Unable to create DirectX swap chain: %d", resultValue);
            GEK_CHECK_EXCEPTION(!d3dDevice, Video::Exception, "Unable to create Direct3D device %d", resultValue);
            GEK_CHECK_EXCEPTION(!d3dDeviceContext, Video::Exception, "Unable to create Direct3D device context: %d", resultValue);

            if (fullScreen && !isChildWindow)
            {
                resultValue = dxSwapChain->SetFullscreenState(true, nullptr);
                GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to set fullscreen state: %d", resultValue);
            }

            defaultContext = std::make_shared<VideoContextImplementation>(d3dDeviceContext);
        }

        ~VideoSystemImplementation(void)
        {
            setFullScreen(false);

            backBuffer = nullptr;
            defaultContext = nullptr;

            dxSwapChain.Release();
            d3dDeviceContext.Release();
            d3dDevice.Release();
        }

        // VideoSystem
        void setFullScreen(bool fullScreen)
        {
            if (!isChildWindow && this->fullScreen != fullScreen)
            {
                this->fullScreen = fullScreen;
                HRESULT resultValue = dxSwapChain->SetFullscreenState(fullScreen, nullptr);
                GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to change fullscreen state (%s): %d", (fullScreen ? "fullscreen" : "windowed"), resultValue);
            }
        }

        void setSize(UINT32 width, UINT32 height, Video::Format format)
        {
            GEK_REQUIRE(dxSwapChain);

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
                HRESULT resultValue = dxSwapChain->ResizeTarget(&description);
                GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to resize swap chain target: %d", resultValue);
            }
        }

        void resize(void)
        {
            GEK_REQUIRE(dxSwapChain);

            backBuffer = nullptr;
            HRESULT resultValue = dxSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
            GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to resize swap chain buffers: %d", resultValue);
        }

        VideoTarget * const getBackBuffer(void)
        {
            if (!backBuffer)
            {
                CComPtr<ID3D11Texture2D> d3dRenderTarget;
                HRESULT resultValue = dxSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                GEK_CHECK_EXCEPTION(!d3dRenderTarget, Video::Exception, "Unable to get swap chain buffer: %d", resultValue);

                CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                GEK_CHECK_EXCEPTION(!d3dRenderTargetView, Video::Exception, "Unable to create view object from swap chain: %d", resultValue);

                D3D11_TEXTURE2D_DESC description;
                d3dRenderTarget->GetDesc(&description);
                backBuffer = std::dynamic_pointer_cast<VideoTarget>(std::make_shared<TargetTexture>(d3dRenderTarget.p, d3dRenderTargetView.p, format, description.Width, description.Height, 1));
            }

            return backBuffer.get();
        }

        VideoContext * const getDefaultContext(void)
        {
            GEK_REQUIRE(defaultContext);

            return defaultContext.get();
        }

        bool isFullScreen(void)
        {
            return fullScreen;
        }

        std::shared_ptr<VideoContext> createDeferredContext(void)
        {
            GEK_REQUIRE(d3dDevice);

            CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
            HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext);
            GEK_CHECK_EXCEPTION(!d3dDeferredDeviceContext, Video::Exception, "Unable to create deferred context: %d", resultValue);

            return std::dynamic_pointer_cast<VideoContext>(std::make_shared<VideoContextImplementation>(d3dDeferredDeviceContext.p));
        }

        std::shared_ptr<VideoObject> createEvent(void)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_QUERY_DESC description;
            description.Query = D3D11_QUERY_EVENT;
            description.MiscFlags = 0;

            CComPtr<ID3D11Query> d3dQuery;
            HRESULT resultValue = d3dDevice->CreateQuery(&description, &d3dQuery);
            GEK_CHECK_EXCEPTION(!d3dQuery, Video::Exception, "Unable to create event object: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<Event>(d3dQuery));
        }

        void setEvent(VideoObject *event)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(event);

            d3dDeviceContext->End(reinterpret_cast<Event *>(event)->d3dQuery);
        }

        bool isEventSet(VideoObject *event)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(event);

            UINT32 isEventSet = 0;
            if (FAILED(d3dDeviceContext->GetData(reinterpret_cast<Event *>(event)->d3dQuery, (LPVOID)&isEventSet, sizeof(UINT32), TRUE)))
            {
                isEventSet = 0;
            }

            return (isEventSet == 1);
        }

        std::shared_ptr<VideoObject> createRenderState(const Video::RenderState &renderState)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_RASTERIZER_DESC rasterizerDescription;
            rasterizerDescription.FrontCounterClockwise = renderState.frontCounterClockwise;
            rasterizerDescription.DepthBias = renderState.depthBias;
            rasterizerDescription.DepthBiasClamp = renderState.depthBiasClamp;
            rasterizerDescription.SlopeScaledDepthBias = renderState.slopeScaledDepthBias;
            rasterizerDescription.DepthClipEnable = renderState.depthClipEnable;
            rasterizerDescription.ScissorEnable = renderState.scissorEnable;
            rasterizerDescription.MultisampleEnable = renderState.multisampleEnable;
            rasterizerDescription.AntialiasedLineEnable = renderState.antialiasedLineEnable;
            rasterizerDescription.FillMode = DirectX::FillModeList[static_cast<UINT8>(renderState.fillMode)];
            rasterizerDescription.CullMode = DirectX::CullModeList[static_cast<UINT8>(renderState.cullMode)];

            CComPtr<ID3D11RasterizerState> d3dStates;
            HRESULT resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates);
            GEK_CHECK_EXCEPTION(!d3dStates, Video::Exception, "Unable to create render state: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<RenderState>(d3dStates));
        }

        std::shared_ptr<VideoObject> createDepthState(const Video::DepthState &depthState)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
            depthStencilDescription.DepthEnable = depthState.enable;
            depthStencilDescription.DepthFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthState.comparisonFunction)];
            depthStencilDescription.StencilEnable = depthState.stencilEnable;
            depthStencilDescription.StencilReadMask = depthState.stencilReadMask;
            depthStencilDescription.StencilWriteMask = depthState.stencilWriteMask;
            depthStencilDescription.FrontFace.StencilFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilFrontState.failOperation)];
            depthStencilDescription.FrontFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilFrontState.depthFailOperation)];
            depthStencilDescription.FrontFace.StencilPassOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilFrontState.passOperation)];
            depthStencilDescription.FrontFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthState.stencilFrontState.comparisonFunction)];
            depthStencilDescription.BackFace.StencilFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilBackState.failOperation)];
            depthStencilDescription.BackFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilBackState.depthFailOperation)];
            depthStencilDescription.BackFace.StencilPassOp = DirectX::StencilOperationList[static_cast<UINT8>(depthState.stencilBackState.passOperation)];
            depthStencilDescription.BackFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(depthState.stencilBackState.comparisonFunction)];
            depthStencilDescription.DepthWriteMask = DirectX::DepthWriteMaskList[static_cast<UINT8>(depthState.writeMask)];

            CComPtr<ID3D11DepthStencilState> d3dStates;
            HRESULT resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates);
            GEK_CHECK_EXCEPTION(!d3dStates, Video::Exception, "Unable to create depth state: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<DepthState>(d3dStates));
        }

        std::shared_ptr<VideoObject> createBlendState(const Video::UnifiedBlendState &blendState)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_BLEND_DESC blendDescription;
            blendDescription.AlphaToCoverageEnable = blendState.alphaToCoverage;
            blendDescription.IndependentBlendEnable = false;
            blendDescription.RenderTarget[0].BlendEnable = blendState.enable;
            blendDescription.RenderTarget[0].SrcBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendState.colorSource)];
            blendDescription.RenderTarget[0].DestBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendState.colorDestination)];
            blendDescription.RenderTarget[0].BlendOp = DirectX::BlendOperationList[static_cast<UINT8>(blendState.colorOperation)];
            blendDescription.RenderTarget[0].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendState.alphaSource)];
            blendDescription.RenderTarget[0].DestBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendState.alphaDestination)];
            blendDescription.RenderTarget[0].BlendOpAlpha = DirectX::BlendOperationList[static_cast<UINT8>(blendState.alphaOperation)];
            blendDescription.RenderTarget[0].RenderTargetWriteMask = 0;
            if (blendState.writeMask & Video::ColorMask::R)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
            }

            if (blendState.writeMask & Video::ColorMask::G)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
            }

            if (blendState.writeMask & Video::ColorMask::B)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
            }

            if (blendState.writeMask & Video::ColorMask::A)
            {
                blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
            }

            CComPtr<ID3D11BlendState> d3dStates;
            HRESULT resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
            GEK_CHECK_EXCEPTION(!d3dStates, Video::Exception, "Unable to create unified blend state: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<BlendState>(d3dStates));
        }

        std::shared_ptr<VideoObject> createBlendState(const Video::IndependentBlendState &blendState)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_BLEND_DESC blendDescription;
            blendDescription.AlphaToCoverageEnable = blendState.alphaToCoverage;
            blendDescription.IndependentBlendEnable = true;
            for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
            {
                blendDescription.RenderTarget[renderTarget].BlendEnable = blendState.targetStates[renderTarget].enable;
                blendDescription.RenderTarget[renderTarget].SrcBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendState.targetStates[renderTarget].colorSource)];
                blendDescription.RenderTarget[renderTarget].DestBlend = DirectX::BlendSourceList[static_cast<UINT8>(blendState.targetStates[renderTarget].colorDestination)];
                blendDescription.RenderTarget[renderTarget].BlendOp = DirectX::BlendOperationList[static_cast<UINT8>(blendState.targetStates[renderTarget].colorOperation)];
                blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendState.targetStates[renderTarget].alphaSource)];
                blendDescription.RenderTarget[renderTarget].DestBlendAlpha = DirectX::BlendSourceList[static_cast<UINT8>(blendState.targetStates[renderTarget].alphaDestination)];
                blendDescription.RenderTarget[renderTarget].BlendOpAlpha = DirectX::BlendOperationList[static_cast<UINT8>(blendState.targetStates[renderTarget].alphaOperation)];
                blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                if (blendState.targetStates[renderTarget].writeMask & Video::ColorMask::R)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                }

                if (blendState.targetStates[renderTarget].writeMask & Video::ColorMask::G)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                }

                if (blendState.targetStates[renderTarget].writeMask & Video::ColorMask::B)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                }

                if (blendState.targetStates[renderTarget].writeMask & Video::ColorMask::A)
                {
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                }
            }

            CComPtr<ID3D11BlendState> d3dStates;
            HRESULT resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
            GEK_CHECK_EXCEPTION(!d3dStates, Video::Exception, "Unable to create independent blend state: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<BlendState>(d3dStates));
        }

        std::shared_ptr<VideoObject> createSamplerState(const Video::SamplerState &samplerState)
        {
            GEK_REQUIRE(d3dDevice);

            D3D11_SAMPLER_DESC samplerDescription;
            samplerDescription.AddressU = DirectX::AddressModeList[static_cast<UINT8>(samplerState.addressModeU)];
            samplerDescription.AddressV = DirectX::AddressModeList[static_cast<UINT8>(samplerState.addressModeV)];
            samplerDescription.AddressW = DirectX::AddressModeList[static_cast<UINT8>(samplerState.addressModeW)];
            samplerDescription.MipLODBias = samplerState.mipLevelBias;
            samplerDescription.MaxAnisotropy = samplerState.maximumAnisotropy;
            samplerDescription.ComparisonFunc = DirectX::ComparisonFunctionList[static_cast<UINT8>(samplerState.comparisonFunction)];
            samplerDescription.BorderColor[0] = samplerState.borderColor.r;
            samplerDescription.BorderColor[1] = samplerState.borderColor.g;
            samplerDescription.BorderColor[2] = samplerState.borderColor.b;
            samplerDescription.BorderColor[3] = samplerState.borderColor.a;
            samplerDescription.MinLOD = samplerState.minimumMipLevel;
            samplerDescription.MaxLOD = samplerState.maximumMipLevel;
            samplerDescription.Filter = DirectX::FilterList[static_cast<UINT8>(samplerState.filterMode)];

            CComPtr<ID3D11SamplerState> d3dStates;
            HRESULT resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates);
            GEK_CHECK_EXCEPTION(!d3dStates, Video::Exception, "Unable to create sampler state: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<SamplerState>(d3dStates));
        }

        std::shared_ptr<VideoBuffer> createBuffer(Video::Format format, UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(stride > 0);
            GEK_REQUIRE(count > 0);

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
                HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer);
                GEK_CHECK_EXCEPTION(!d3dBuffer, Video::Exception, "Unable to create dynamic buffer: %d", resultValue);
            }
            else
            {
                D3D11_SUBRESOURCE_DATA resourceData;
                resourceData.pSysMem = data;
                resourceData.SysMemPitch = 0;
                resourceData.SysMemSlicePitch = 0;
                HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, &resourceData, &d3dBuffer);
                GEK_CHECK_EXCEPTION(!d3dBuffer, Video::Exception, "Unable to create static buffer: %d", resultValue);
            }

            CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
            if (flags & Video::BufferFlags::Resource)
            {
                D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                viewDescription.Format = DirectX::BufferFormatList[static_cast<UINT8>(format)];
                viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                viewDescription.Buffer.FirstElement = 0;
                viewDescription.Buffer.NumElements = count;
                HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                GEK_CHECK_EXCEPTION(!d3dShaderResourceView, Video::Exception, "Unable to create buffer resource view: %d", resultValue);
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

                HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                GEK_CHECK_EXCEPTION(!d3dUnorderedAccessView, Video::Exception, "Unable to create buffer unordered access view: %d", resultValue);
            }

            return std::dynamic_pointer_cast<VideoBuffer>(std::make_shared<ViewBuffer>(d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView, format, stride, count));
        }

        std::shared_ptr<VideoBuffer> createBuffer(UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            return createBuffer(Video::Format::Unknown, stride, count, type, flags, data);
        }

        std::shared_ptr<VideoBuffer> createBuffer(Video::Format format, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            UINT32 stride = DirectX::FormatStrideList[static_cast<UINT8>(format)];
            return createBuffer(format, stride, count, type, flags, data);
        }

        void updateBuffer(VideoBuffer *buffer, LPCVOID data)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(data);

            d3dDeviceContext->UpdateSubresource(reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer, 0, nullptr, data, 0, 0);
        }

        void mapBuffer(VideoBuffer *buffer, void **data, Video::Map mapping)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(data);

            D3D11_MAP d3dMapping = DirectX::MapList[static_cast<UINT8>(mapping)];

            D3D11_MAPPED_SUBRESOURCE mappedSubResource;
            mappedSubResource.pData = nullptr;
            mappedSubResource.RowPitch = 0;
            mappedSubResource.DepthPitch = 0;

            HRESULT resultValue = d3dDeviceContext->Map(reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer, 0, d3dMapping, 0, &mappedSubResource);
            GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to map buffer: %d", resultValue);

            (*data) = mappedSubResource.pData;
        }

        void unmapBuffer(VideoBuffer *buffer)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(buffer);

            d3dDeviceContext->Unmap(reinterpret_cast<BaseBuffer *>(buffer)->d3dBuffer, 0);
        }

        void copyResource(VideoObject *destination, VideoObject *source)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(destination);
            GEK_REQUIRE(source);

            d3dDeviceContext->CopyResource(reinterpret_cast<Resource *>(destination)->d3dResource, reinterpret_cast<Resource *>(source)->d3dResource);
        }

        std::shared_ptr<VideoObject> compileComputeProgram(LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::unordered_map<CStringA, CStringA> &defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(programScript);
            GEK_REQUIRE(entryFunction);

            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            for (auto &define : defineList)
            {
                D3D_SHADER_MACRO d3dShaderMacro = { define.first.GetString(), define.second.GetString() };
                d3dShaderMacroList.push_back(d3dShaderMacro);
            }

            d3dShaderMacroList.push_back({ "_COMPUTE_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            HRESULT resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "cs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            GEK_CHECK_EXCEPTION(!d3dShaderBlob, Video::Exception, "Unable to compile shader data blob: %d", resultValue);

            CComPtr<ID3D11ComputeShader> d3dShader;
            resultValue = d3dDevice->CreateComputeShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
            GEK_CHECK_EXCEPTION(!d3dShader, Video::Exception, "Unable to create shader: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<ComputeProgram>(d3dShader));
        }

        std::shared_ptr<VideoObject> compileVertexProgram(LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout, const std::unordered_map<CStringA, CStringA> &defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(programScript);
            GEK_REQUIRE(entryFunction);

            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            for (auto &kPair : defineList)
            {
                D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                d3dShaderMacroList.push_back(d3dShaderMacro);
            }

            d3dShaderMacroList.push_back({ "_VERTEX_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            HRESULT resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "vs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            GEK_CHECK_EXCEPTION(!d3dShaderBlob, Video::Exception, "Unable to compile shader data blob: %d", resultValue);

            CComPtr<ID3D11VertexShader> d3dShader;
            resultValue = d3dDevice->CreateVertexShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
            GEK_CHECK_EXCEPTION(!d3dShader, Video::Exception, "Unable to create shader: %d", resultValue);

            CComPtr<ID3D11InputLayout> d3dInputLayout;
            if (!elementLayout.empty())
            {
                Video::ElementType lastElementType = Video::ElementType::Vertex;
                std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementList;
                for (auto &element : elementLayout)
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
                        break;
                    };

                    elementDesc.Format = DirectX::BufferFormatList[static_cast<UINT8>(element.format)];
                    inputElementList.push_back(elementDesc);
                }

                resultValue = d3dDevice->CreateInputLayout(inputElementList.data(), inputElementList.size(), d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), &d3dInputLayout);
                GEK_CHECK_EXCEPTION(!d3dInputLayout, Video::Exception, "Unable to create vertex input layout: %d", resultValue);
            }

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<VertexProgram>(d3dShader, d3dInputLayout));
        }

        std::shared_ptr<VideoObject> compileGeometryProgram(LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::unordered_map<CStringA, CStringA> &defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(programScript);
            GEK_REQUIRE(entryFunction);

            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            for (auto &kPair : defineList)
            {
                D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                d3dShaderMacroList.push_back(d3dShaderMacro);
            }

            d3dShaderMacroList.push_back({ "_GEOMETRY_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            HRESULT resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "gs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            GEK_CHECK_EXCEPTION(!d3dShaderBlob, Video::Exception, "Unable to compile shader data blob: %d", resultValue);

            CComPtr<ID3D11GeometryShader> d3dShader;
            resultValue = d3dDevice->CreateGeometryShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
            GEK_CHECK_EXCEPTION(!d3dShader, Video::Exception, "Unable to create shader: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<GeometryProgram>(d3dShader));
        }

        std::shared_ptr<VideoObject> compilePixelProgram(LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::unordered_map<CStringA, CStringA> &defineList, ID3DInclude *includes)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(programScript);
            GEK_REQUIRE(entryFunction);

            UINT32 flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
            flags |= D3DCOMPILE_DEBUG;
#endif

            std::vector<D3D_SHADER_MACRO> d3dShaderMacroList;
            for (auto &kPair : defineList)
            {
                D3D_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
                d3dShaderMacroList.push_back(d3dShaderMacro);
            }

            d3dShaderMacroList.push_back({ "_PIXEL_PROGRAM", "1" });
            d3dShaderMacroList.push_back({ nullptr, nullptr });

            CComPtr<ID3DBlob> d3dShaderBlob;
            CComPtr<ID3DBlob> d3dCompilerErrors;
            HRESULT resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "ps_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
            GEK_CHECK_EXCEPTION(!d3dShaderBlob, Video::Exception, "Unable to compile shader data blob: %d", resultValue);

            CComPtr<ID3D11PixelShader> d3dShader;
            resultValue = d3dDevice->CreatePixelShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader);
            GEK_CHECK_EXCEPTION(!d3dShader, Video::Exception, "Unable to create shader: %d", resultValue);

            return std::dynamic_pointer_cast<VideoObject>(std::make_shared<PixelProgram>(d3dShader));
        }

        std::shared_ptr<VideoObject> compileComputeProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            IncludeImplementation includeHandler(L"", onInclude);
            return compileComputeProgram(nullptr, programScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> compileVertexProgram(LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            IncludeImplementation includeHandler(L"", onInclude);
            return compileVertexProgram(nullptr, programScript, entryFunction, elementLayout, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> compileGeometryProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            IncludeImplementation includeHandler(L"", onInclude);
            return compileGeometryProgram(nullptr, programScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> compilePixelProgram(LPCSTR programScript, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            IncludeImplementation includeHandler(L"", onInclude);
            return compilePixelProgram(nullptr, programScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> loadComputeProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            GEK_REQUIRE(fileName);

            CStringA progamScript;
            Gek::FileSystem::load(fileName, progamScript);

            IncludeImplementation includeHandler(L"", onInclude);
            return compileComputeProgram(fileName, progamScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> loadVertexProgram(LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Video::InputElement> &elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            GEK_REQUIRE(fileName);

            CStringA progamScript;
            Gek::FileSystem::load(fileName, progamScript);

            IncludeImplementation includeHandler(L"", onInclude);
            return compileVertexProgram(fileName, progamScript, entryFunction, elementLayout, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> loadGeometryProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            GEK_REQUIRE(fileName);

            CStringA progamScript;
            Gek::FileSystem::load(fileName, progamScript);

            IncludeImplementation includeHandler(L"", onInclude);
            return compileGeometryProgram(fileName, progamScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoObject> loadPixelProgram(LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, const std::unordered_map<CStringA, CStringA> &defineList)
        {
            GEK_REQUIRE(fileName);

            CStringA progamScript;
            Gek::FileSystem::load(fileName, progamScript);

            IncludeImplementation includeHandler(L"", onInclude);
            return compilePixelProgram(fileName, progamScript, entryFunction, defineList, &includeHandler);
        }

        std::shared_ptr<VideoTexture> createTexture(Video::Format format, UINT32 width, UINT32 height, UINT32 depth, UINT32 flags, UINT32 mipmaps)
        {
            GEK_REQUIRE(d3dDevice);

            UINT32 bindFlags = 0;
            if (flags & Video::TextureFlags::RenderTarget)
            {
                GEK_CHECK_EXCEPTION(flags & Video::TextureFlags::DepthTarget, Video::Exception, "Render/DepthTarget flags are mutually exclusive");
                bindFlags |= D3D11_BIND_RENDER_TARGET;
            }

            if (flags & Video::TextureFlags::DepthTarget)
            {
                GEK_CHECK_EXCEPTION(flags & Video::TextureFlags::RenderTarget, Video::Exception, "Render/DepthTarget flags are mutually exclusive");
                GEK_CHECK_EXCEPTION(depth > 1, Video::Exception, "Depth target can not have depth greater than 1: %d required", depth);
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
                HRESULT resultValue = d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture2D);
                GEK_CHECK_EXCEPTION(!texture2D, Video::Exception, "Unable to create 2D texture: %d", resultValue);
                d3dResource = texture2D;
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

                CComPtr<ID3D11Texture3D> texture3D;
                HRESULT resultValue = d3dDevice->CreateTexture3D(&textureDescription, nullptr, &texture3D);
                GEK_CHECK_EXCEPTION(!texture3D, Video::Exception, "Unable to create 3D texture: %d", resultValue);
                d3dResource = texture3D;
            }

            GEK_CHECK_EXCEPTION(!d3dResource, Video::Exception, "Unable to get texture resource");

            CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
            if (flags & Video::TextureFlags::Resource)
            {
                HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dResource, nullptr, &d3dShaderResourceView);
                GEK_CHECK_EXCEPTION(!d3dShaderResourceView, Video::Exception, "Unable to create resource view: %d", resultValue);
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

                HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView);
                GEK_CHECK_EXCEPTION(!d3dUnorderedAccessView, Video::Exception, "Unable to create unordered access view: %d", resultValue);
            }

            if (flags & Video::TextureFlags::RenderTarget)
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

                CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                HRESULT resultValue = d3dDevice->CreateRenderTargetView(d3dResource, &renderViewDescription, &d3dRenderTargetView);
                GEK_CHECK_EXCEPTION(!d3dRenderTargetView, Video::Exception, "Unable to create render target view: %d", resultValue);

                return std::dynamic_pointer_cast<VideoTexture>(std::make_shared<TargetViewTexture>(d3dResource.p, d3dRenderTargetView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth));
            }
            else if (flags & Video::TextureFlags::DepthTarget)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                depthStencilDescription.Format = d3dFormat;
                depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                depthStencilDescription.Flags = 0;
                depthStencilDescription.Texture2D.MipSlice = 0;

                CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
                HRESULT resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &d3dDepthStencilView);
                GEK_CHECK_EXCEPTION(!d3dDepthStencilView, Video::Exception, "Unable to create depth stencil view: %d", resultValue);

                return std::dynamic_pointer_cast<VideoTexture>(std::make_shared<DepthTexture>(d3dResource.p, d3dDepthStencilView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth));
            }
            else
            {
                return std::dynamic_pointer_cast<VideoTexture>(std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth));
            }
        }

        std::shared_ptr<VideoTexture> loadTexture(LPCWSTR fileName, UINT32 flags)
        {
            GEK_REQUIRE(d3dDevice);
            GEK_REQUIRE(fileName);

            std::vector<UINT8> fileData;
            Gek::FileSystem::load(fileName, fileData);

            ::DirectX::ScratchImage scratchImage;
            ::DirectX::TexMetadata textureMetaData;

            CPathW filePath(fileName);
            CStringW extension(filePath.GetExtension());
            std::function<HRESULT(UINT8*, size_t, ::DirectX::TexMetadata *, ::DirectX::ScratchImage &)> load;
            if (extension.CompareNoCase(L".dds") == 0)
            {
                load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, std::placeholders::_3, std::placeholders::_4);
            }
            else if (extension.CompareNoCase(L".tga") == 0)
            {
                load = std::bind(::DirectX::LoadFromTGAMemory, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            }
            else if (extension.CompareNoCase(L".png") == 0)
            {
                load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_PNG, std::placeholders::_3, std::placeholders::_4);
            }
            else if (extension.CompareNoCase(L".bmp") == 0)
            {
                load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_BMP, std::placeholders::_3, std::placeholders::_4);
            }
            else if (extension.CompareNoCase(L".jpg") == 0 ||
                extension.CompareNoCase(L".jpeg") == 0)
            {
                load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_JPEG, std::placeholders::_3, std::placeholders::_4);
            }
            
            GEK_CHECK_EXCEPTION(!load, Video::Exception, "Invalid file type: %s", extension);
            HRESULT resultValue = load(fileData.data(), fileData.size(), &textureMetaData, scratchImage);
            GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to load file: %d", resultValue);

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
            resultValue = ::DirectX::CreateShaderResourceView(d3dDevice, scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), &d3dShaderResourceView);
            GEK_CHECK_EXCEPTION(!d3dShaderResourceView, Video::Exception, "Unable to create resource view: %d", resultValue);

            CComPtr<ID3D11Resource> d3dResource;
            d3dShaderResourceView->GetResource(&d3dResource);
            GEK_CHECK_EXCEPTION(!d3dResource, Video::Exception, "Unable to get resource object");

            return std::dynamic_pointer_cast<VideoTexture>(std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, nullptr, Video::Format::Unknown, scratchImage.GetMetadata().width, scratchImage.GetMetadata().height, scratchImage.GetMetadata().depth));
        }

        std::shared_ptr<VideoTexture> loadCubeMap(LPCWSTR fileNameList[6], UINT32 flags)
        {
            GEK_REQUIRE(d3dDevice);

            ::DirectX::ScratchImage cubeMapList[6];
            ::DirectX::TexMetadata cubeMapMetaData;
            for (UINT32 side = 0; side < 6; side++)
            {
                std::vector<UINT8> fileData;
                Gek::FileSystem::load(fileNameList[side], fileData);

                CPathW filePath(fileNameList[side]);
                CStringW extension(filePath.GetExtension());
                std::function<HRESULT(UINT8*, size_t, ::DirectX::TexMetadata *, ::DirectX::ScratchImage &)> load;
                if (extension.CompareNoCase(L".dds") == 0)
                {
                    load = std::bind(::DirectX::LoadFromDDSMemory, std::placeholders::_1, std::placeholders::_2, 0, std::placeholders::_3, std::placeholders::_4);
                }
                else if (extension.CompareNoCase(L".tga") == 0)
                {
                    load = std::bind(::DirectX::LoadFromTGAMemory, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
                }
                else if (extension.CompareNoCase(L".png") == 0)
                {
                    load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_PNG, std::placeholders::_3, std::placeholders::_4);
                }
                else if (extension.CompareNoCase(L".bmp") == 0)
                {
                    load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_BMP, std::placeholders::_3, std::placeholders::_4);
                }
                else if (extension.CompareNoCase(L".jpg") == 0 ||
                    extension.CompareNoCase(L".jpeg") == 0)
                {
                    load = std::bind(::DirectX::LoadFromWICMemory, std::placeholders::_1, std::placeholders::_2, ::DirectX::WIC_CODEC_JPEG, std::placeholders::_3, std::placeholders::_4);
                }

                GEK_CHECK_EXCEPTION(!load, Video::Exception, "Invalid file type: %s", extension);
                HRESULT resultValue = load(fileData.data(), fileData.size(), &cubeMapMetaData, cubeMapList[side]);
                GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to load file: %d", resultValue);
            }

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
            HRESULT resultValue = cubeMap.InitializeCubeFromImages(imageList, 6, 0);
            GEK_CHECK_EXCEPTION(FAILED(resultValue), Video::Exception, "Unable to create cubemap from images: %d", resultValue);

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
            GEK_CHECK_EXCEPTION(!d3dShaderResourceView, Video::Exception, "Unable to create resource view: %d", resultValue);

            CComPtr<ID3D11Resource> d3dResource;
            d3dShaderResourceView->GetResource(&d3dResource);
            GEK_CHECK_EXCEPTION(!d3dResource, Video::Exception, "Unable to get resource object");

            return std::dynamic_pointer_cast<VideoTexture>(std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, nullptr, Video::Format::Unknown, cubeMapMetaData.width, cubeMapMetaData.height, 1));
        }

        void updateTexture(VideoTexture *texture, LPCVOID data, UINT32 pitch, Shapes::Rectangle<UINT32> *destinationRectangle)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(texture);
            GEK_REQUIRE(data);
            GEK_REQUIRE(pitch);

            if (destinationRectangle == nullptr)
            {
                d3dDeviceContext->UpdateSubresource(reinterpret_cast<BaseTexture *>(texture)->d3dResource, 0, nullptr, data, pitch, pitch);
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

                d3dDeviceContext->UpdateSubresource(reinterpret_cast<BaseTexture *>(texture)->d3dResource, 0, &destinationBox, data, pitch, pitch);
            }
        }

        void executeCommandList(VideoObject *commandList)
        {
            GEK_REQUIRE(d3dDeviceContext);
            GEK_REQUIRE(commandList);

            CComQIPtr<ID3D11CommandList> d3dCommandList;
            d3dDeviceContext->ExecuteCommandList(reinterpret_cast<CommandList *>(commandList)->d3dCommandList, FALSE);
        }

        void present(bool waitForVerticalSync)
        {
            GEK_REQUIRE(dxSwapChain);

            dxSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
        }
    };

    GEK_REGISTER_PLUGIN(VideoSystemImplementation);
}; // namespace Gek
