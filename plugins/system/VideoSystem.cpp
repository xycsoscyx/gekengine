#pragma warning(disable : 4005)

#include "GEK\Context\Common.h"
#include "GEK\Context\UnknownMixin.h"
#include "GEK\Context\ContextUserMixin.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\System\VideoSystem.h"
#include <atlbase.h>
#include <atlpath.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>
#include <memory>
#include <wincodec.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

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

        static const DWRITE_FONT_STYLE FontStyleList[] =
        {
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STYLE_ITALIC,
        };

        static const D2D1_INTERPOLATION_MODE InterpolationMode[] =
        {
            D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_INTERPOLATION_MODE_LINEAR,
            D2D1_INTERPOLATION_MODE_CUBIC,
            D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR,
            D2D1_INTERPOLATION_MODE_ANISOTROPIC,
            D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,
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
        CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
        CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
        Video::Format format;
        UINT32 width;
        UINT32 height;
        UINT32 depth;
        Video::ViewPort viewPort;

    public:
        TextureImplementation(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, ID3D11DepthStencilView *d3dDepthStencilView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, UINT32 width, UINT32 height, UINT32 depth)
            : d3dResource(d3dResource)
            , d3dRenderTargetView(d3dRenderTargetView)
            , d3dDepthStencilView(d3dDepthStencilView)
            , d3dShaderResourceView(d3dShaderResourceView)
            , d3dUnorderedAccessView(d3dUnorderedAccessView)
            , format(format)
            , width(width)
            , height(height)
            , depth(depth)
            , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(width), float(height)), 0.0f, 1.0f)
        {
        }

        BEGIN_INTERFACE_LIST(TextureImplementation)
            INTERFACE_LIST_ENTRY_COM(VideoTexture)
            INTERFACE_LIST_ENTRY_COM(VideoTarget)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Resource, d3dResource)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, d3dRenderTargetView)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DepthStencilView, d3dDepthStencilView)
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

    class GeometryImplementation : public UnknownMixin
        , public OverlayGeometry
    {
    private:
        CComPtr<ID2D1PathGeometry> d2dPathGeometry;
        CComPtr<ID2D1GeometrySink> d2dGeometrySink;
        bool figureHasBegun;

    public:
        GeometryImplementation(ID2D1PathGeometry *d2dPathGeometry)
            : d2dPathGeometry(d2dPathGeometry)
            , figureHasBegun(false)
        {
        }

        BEGIN_INTERFACE_LIST(GeometryImplementation)
            INTERFACE_LIST_ENTRY_COM(OverlayGeometry)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID2D1PathGeometry, d2dPathGeometry)
        END_INTERFACE_LIST_UNKNOWN

        // OverlayGeometry
        STDMETHODIMP openShape(void)
        {
            REQUIRE_RETURN(d2dGeometrySink == nullptr, E_FAIL);
            REQUIRE_RETURN(d2dPathGeometry, E_FAIL);

            d2dGeometrySink = nullptr;
            return d2dPathGeometry->Open(&d2dGeometrySink);
        }

        STDMETHODIMP closeShape(void)
        {
            REQUIRE_RETURN(d2dGeometrySink, E_FAIL);

            CComPtr<ID2D1GeometrySink> d2dCopyGeometrySink(d2dGeometrySink);
            d2dGeometrySink = nullptr;
            return d2dCopyGeometrySink->Close();
        }

        STDMETHODIMP_(void) beginModifications(const Math::Float2 &point, bool fillShape)
        {
            REQUIRE_VOID_RETURN(d2dGeometrySink);

            if (!figureHasBegun)
            {
                figureHasBegun = true;
                d2dGeometrySink->BeginFigure(*(D2D1_POINT_2F *)&point, fillShape ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
            }
        }

        STDMETHODIMP_(void) endModifications(bool openEnded)
        {
            REQUIRE_VOID_RETURN(d2dGeometrySink);

            if (figureHasBegun)
            {
                figureHasBegun = false;
                d2dGeometrySink->EndFigure(openEnded ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);
            }
        }

        STDMETHODIMP_(void) addLine(const Math::Float2 &point)
        {
            REQUIRE_VOID_RETURN(d2dGeometrySink);

            if (figureHasBegun)
            {
                d2dGeometrySink->AddLine(*(D2D1_POINT_2F *)&point);
            }
        }

        STDMETHODIMP_(void) addBezier(const Math::Float2 &point1, const Math::Float2 &point2, const Math::Float2 &point3)
        {
            REQUIRE_VOID_RETURN(d2dGeometrySink);

            if (figureHasBegun)
            {
                d2dGeometrySink->AddBezier({ *(D2D1_POINT_2F *)&point1, *(D2D1_POINT_2F *)&point2, *(D2D1_POINT_2F *)&point3 });
            }
        }

        STDMETHODIMP createWidened(float width, float tolerance, OverlayGeometry **returnObject)
        {
            REQUIRE_RETURN(d2dPathGeometry, E_INVALIDARG);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);

            HRESULT resultValue = E_FAIL;
            CComPtr<ID2D1Factory> d2dFactory;
            d2dPathGeometry->GetFactory(&d2dFactory);
            if (d2dFactory)
            {
                CComPtr<ID2D1PathGeometry> d2dWidePathGeometry;
                resultValue = d2dFactory->CreatePathGeometry(&d2dWidePathGeometry);
                if (d2dWidePathGeometry)
                {
                    CComPtr<ID2D1GeometrySink> d2dWideGeometrySink;
                    resultValue = d2dWidePathGeometry->Open(&d2dWideGeometrySink);
                    if (d2dWideGeometrySink)
                    {
                        resultValue = d2dPathGeometry->Widen(width, nullptr, nullptr, tolerance, d2dWideGeometrySink);
                        if (SUCCEEDED(resultValue))
                        {
                            d2dWideGeometrySink->Close();
                            resultValue = E_OUTOFMEMORY;
                            CComPtr<GeometryImplementation> geometry(new GeometryImplementation(d2dWidePathGeometry));
                            if (geometry)
                            {
                                resultValue = geometry->QueryInterface(IID_PPV_ARGS(returnObject));
                            }
                        }
                    }
                }
            }

            return resultValue;
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
            REQUIRE_RETURN(fileName, E_INVALIDARG);
            REQUIRE_RETURN(data, E_INVALIDARG);
            REQUIRE_RETURN(dataSize, E_INVALIDARG);

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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ComputeShader> d3dComputeShader(program);
            d3dDeviceContext->CSSetShader(d3dComputeShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->CSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->CSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
            d3dDeviceContext->CSSetShaderResources(stage, 1, &d3dShaderResourceView.p);
        }

        STDMETHODIMP_(void) setUnorderedAccess(IUnknown *unorderedAccess, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);

            CComQIPtr<ID3D11VertexShader> d3dVertexShader(program);
            d3dDeviceContext->VSSetShader(d3dVertexShader, nullptr, 0);

            CComQIPtr<ID3D11InputLayout> d3dInputLayout(program);
            d3dDeviceContext->IASetInputLayout(d3dInputLayout);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->VSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->VSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11GeometryShader> d3dGeometryShader(program);
            d3dDeviceContext->GSSetShader(d3dGeometryShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->GSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->GSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11PixelShader> d3dPixelShader(program);
            d3dDeviceContext->PSSetShader(d3dPixelShader, nullptr, 0);
        }

        STDMETHODIMP_(void) setConstantBuffer(VideoBuffer *buffer, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->PSSetConstantBuffers(stage, 1, &d3dBuffer.p);
        }

        STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
            d3dDeviceContext->PSSetSamplers(stage, 1, &d3dSamplerState.p);
        }

        STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
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
            REQUIRE_RETURN(computeSystemHandler, nullptr);
            return computeSystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) vertexPipeline(void)
        {
            REQUIRE_RETURN(vertexSystemHandler, nullptr);
            return vertexSystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) geometryPipeline(void)
        {
            REQUIRE_RETURN(geomtrySystemHandler, nullptr);
            return geomtrySystemHandler.p;
        }

        STDMETHODIMP_(VideoPipeline *) pixelPipeline(void)
        {
            REQUIRE_RETURN(pixelSystemHandler, nullptr);
            return pixelSystemHandler.p;
        }

        STDMETHODIMP_(void) generateMipMaps(VideoTexture *texture)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(texture);

            CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(texture);
            if (d3dShaderResourceView)
            {
                d3dDeviceContext->GenerateMips(d3dShaderResourceView);
            }
        }

        STDMETHODIMP_(void) clearResources(void)
        {
            static ID3D11ShaderResourceView *const d3dShaderResourceViewList[10] = { nullptr };
            static ID3D11UnorderedAccessView *const d3dUnorderedAccessViewList[7] = { nullptr };
            static ID3D11RenderTargetView  *const d3dRenderTargetViewList[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { nullptr };

            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->CSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->CSSetUnorderedAccessViews(0, 7, d3dUnorderedAccessViewList, nullptr);
            d3dDeviceContext->VSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->GSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->PSSetShaderResources(0, 10, d3dShaderResourceViewList);
            d3dDeviceContext->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, d3dRenderTargetViewList, nullptr);
        }

        STDMETHODIMP_(void) setViewports(Video::ViewPort *viewPortList, UINT32 viewPortCount)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(viewPortList);
            REQUIRE_VOID_RETURN(viewPortCount);
            d3dDeviceContext->RSSetViewports(viewPortCount, (D3D11_VIEWPORT *)viewPortList);
        }

        STDMETHODIMP_(void) setScissorRect(Shapes::Rectangle<UINT32> *rectangleList, UINT32 rectangleCount)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(rectangleList);
            REQUIRE_VOID_RETURN(rectangleCount);
            d3dDeviceContext->RSSetScissorRects(rectangleCount, (D3D11_RECT *)rectangleList);
        }

        STDMETHODIMP_(void) clearRenderTarget(VideoTarget *renderTarget, const Math::Color &colorClear)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11RenderTargetView> d3dRenderTargetView(renderTarget);
            if (d3dRenderTargetView)
            {
                d3dDeviceContext->ClearRenderTargetView(d3dRenderTargetView, colorClear.data);
            }
        }

        STDMETHODIMP_(void) clearDepthStencilTarget(IUnknown *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(buffer);

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

        STDMETHODIMP_(void) setRenderTargets(VideoTarget **renderTargetList, UINT32 renderTargetCount, IUnknown *depthBuffer)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(renderTargetList);

            static ID3D11RenderTargetView *d3dRenderTargetViewList[8];
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11RasterizerState> d3dRasterizerState(renderStates);
            d3dDeviceContext->RSSetState(d3dRasterizerState);
        }

        STDMETHODIMP_(void) setDepthStates(IUnknown *depthStates, UINT32 stencilReference)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11DepthStencilState> d3dDepthStencilState(depthStates);
            d3dDeviceContext->OMSetDepthStencilState(d3dDepthStencilState, stencilReference);
        }

        STDMETHODIMP_(void) setBlendStates(IUnknown *blendStates, const Math::Color &blendFactor, UINT32 mask)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11BlendState> d3dBlendState(blendStates);
            d3dDeviceContext->OMSetBlendState(d3dBlendState, blendFactor.data, mask);
        }

        STDMETHODIMP_(void) setVertexBuffer(UINT32 slot, VideoBuffer *vertexBuffer, UINT32 offset)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(vertexBuffer);
            UINT32 stride = (vertexBuffer ? vertexBuffer->getStride() : 0);
            d3dDeviceContext->IASetVertexBuffers(slot, 1, &d3dBuffer.p, &stride, &offset);
        }

        STDMETHODIMP_(void) setIndexBuffer(VideoBuffer *indexBuffer, UINT32 offset)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Buffer> d3dBuffer(indexBuffer);
            DXGI_FORMAT format = (indexBuffer ? (indexBuffer->getFormat() == Video::Format::Short ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT) : DXGI_FORMAT_UNKNOWN);
            d3dDeviceContext->IASetIndexBuffer(d3dBuffer, format, offset);
        }

        STDMETHODIMP_(void) setPrimitiveType(Video::PrimitiveType primitiveType)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->IASetPrimitiveTopology(DirectX::TopologList[static_cast<UINT8>(primitiveType)]);
        }

        STDMETHODIMP_(void) drawPrimitive(UINT32 vertexCount, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->Draw(vertexCount, firstVertex);
        }

        STDMETHODIMP_(void) drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
        }

        STDMETHODIMP_(void) drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
        }

        STDMETHODIMP_(void) drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
        }

        STDMETHODIMP_(void) dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            d3dDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        STDMETHODIMP_(void) finishCommandList(IUnknown **returnObject)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext && returnObject);

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
        , virtual public OverlaySystem
    {
    private:
        HWND window;
        bool isChildWindow;
        bool fullScreen;
        UINT32 width;
        UINT32 height;
        DXGI_FORMAT depthFormat;

        CComPtr<ID3D11Device> d3dDevice;
        CComPtr<IDXGISwapChain> dxSwapChain;
        CComPtr<ID3D11RenderTargetView> d3dDefaultRenderTargetView;
        CComPtr<ID3D11DepthStencilView> d3dDefaultDepthStencilView;

        CComPtr<ID2D1Factory1> d2dFactory;
        CComPtr<ID2D1DeviceContext> d2dDeviceContext;
        CComPtr<IDWriteFactory> dwFactory;

    public:
        VideoSystemImplementation(void)
            : window(nullptr)
            , isChildWindow(false)
            , fullScreen(false)
            , width(0)
            , height(0)
            , depthFormat(DXGI_FORMAT_UNKNOWN)
        {
        }

        ~VideoSystemImplementation(void)
        {
            dwFactory.Release();
            d2dDeviceContext.Release();
            d2dFactory.Release();

            d3dDefaultDepthStencilView.Release();
            d3dDefaultRenderTargetView.Release();
            dxSwapChain.Release();
            d3dDevice.Release();
        }

        BEGIN_INTERFACE_LIST(VideoSystemImplementation)
            INTERFACE_LIST_ENTRY_COM(OverlaySystem)
            INTERFACE_LIST_ENTRY_COM(VideoSystem)
            INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, d3dDevice)
        END_INTERFACE_LIST_USER

        HRESULT createDefaultTargets(void)
        {
            gekCheckScope(resultValue);

            CComPtr<IDXGISurface> dxSurface;
            gekCheckResult(resultValue = dxSwapChain->GetBuffer(0, IID_PPV_ARGS(&dxSurface)));
            if (dxSurface)
            {
                FLOAT desktopHorizontalDPI = 0.0f;
                FLOAT desktopVerticalDPI = 0.0f;
                d2dFactory->GetDesktopDpi(&desktopHorizontalDPI, &desktopVerticalDPI);

                D2D1_BITMAP_PROPERTIES1 desktopProperties;
                desktopProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
                desktopProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
                desktopProperties.dpiX = desktopHorizontalDPI;
                desktopProperties.dpiY = desktopVerticalDPI;
                desktopProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
                desktopProperties.colorContext = nullptr;

                CComPtr<ID2D1Bitmap1> d2dBitmap;
                gekCheckResult(resultValue = d2dDeviceContext->CreateBitmapFromDxgiSurface(dxSurface, &desktopProperties, &d2dBitmap));
                if (d2dBitmap)
                {
                    d2dDeviceContext->SetTarget(d2dBitmap);
                }
            }

            if (SUCCEEDED(resultValue))
            {
                CComPtr<ID3D11Texture2D> d3dRenderTarget;
                gekCheckResult(resultValue = dxSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget)));
                if (d3dRenderTarget)
                {
                    gekCheckResult(resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dDefaultRenderTargetView));
                }
            }

            if (SUCCEEDED(resultValue) && depthFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_TEXTURE2D_DESC depthDescription;
                depthDescription.Format = DXGI_FORMAT_UNKNOWN;
                depthDescription.Width = width;
                depthDescription.Height = height;
                depthDescription.MipLevels = 1;
                depthDescription.ArraySize = 1;
                depthDescription.SampleDesc.Count = 1;
                depthDescription.SampleDesc.Quality = 0;
                depthDescription.Usage = D3D11_USAGE_DEFAULT;
                depthDescription.BindFlags = D3D11_BIND_DEPTH_STENCIL;
                depthDescription.CPUAccessFlags = 0;
                depthDescription.MiscFlags = 0;
                depthDescription.Format = depthFormat;
                if (depthDescription.Format != DXGI_FORMAT_UNKNOWN)
                {
                    CComPtr<ID3D11Texture2D> d3dDepthTarget;
                    gekCheckResult(resultValue = d3dDevice->CreateTexture2D(&depthDescription, nullptr, &d3dDepthTarget));
                    if (d3dDepthTarget)
                    {
                        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                        depthStencilDescription.Format = depthDescription.Format;
                        depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        depthStencilDescription.Flags = 0;
                        depthStencilDescription.Texture2D.MipSlice = 0;
                        gekCheckResult(resultValue = d3dDevice->CreateDepthStencilView(d3dDepthTarget, &depthStencilDescription, &d3dDefaultDepthStencilView));
                        if (d3dDefaultDepthStencilView)
                        {
                            ID3D11RenderTargetView *d3dRenderTargetViewList[1] = { d3dDefaultRenderTargetView };
                            d3dDeviceContext->OMSetRenderTargets(1, d3dRenderTargetViewList, d3dDefaultDepthStencilView);
                        }
                    }
                }
            }

            return resultValue;
        }

        // VideoSystem
        STDMETHODIMP initialize(HWND window, bool fullScreen, Video::Format depthFormat)
        {
            REQUIRE_RETURN(window, E_INVALIDARG);

            gekCheckScope(resultValue, LPCVOID(window), fullScreen, UINT32(depthFormat));

            RECT clientRect;
            GetClientRect(window, &clientRect);
            this->width = (clientRect.right - clientRect.left);
            this->height = (clientRect.bottom - clientRect.top);
            this->isChildWindow = (GetParent(window) != nullptr);
            this->fullScreen = (isChildWindow ? false : fullScreen);
            this->window = window;
            this->depthFormat = DirectX::TextureFormatList[static_cast<UINT8>(depthFormat)];

            DXGI_SWAP_CHAIN_DESC swapChainDescription;
            swapChainDescription.BufferDesc.Width = width;
            swapChainDescription.BufferDesc.Height = height;
            swapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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
            //flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            D3D_FEATURE_LEVEL featureLevelList[] =
            {
                D3D_FEATURE_LEVEL_11_0,
            };

            D3D_FEATURE_LEVEL featureLevel;
            gekCheckResult(resultValue = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, _ARRAYSIZE(featureLevelList), D3D11_SDK_VERSION, &swapChainDescription, &dxSwapChain, &d3dDevice, &featureLevel, &d3dDeviceContext));
            if (d3dDevice && d3dDeviceContext && dxSwapChain)
            {
                gekCheckResult(resultValue = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&d2dFactory)));
                if (d2dFactory)
                {
                    resultValue = E_FAIL;
                    CComQIPtr<IDXGIDevice1> dxDevice(d3dDevice);
                    if (dxDevice)
                    {
                        CComPtr<IDXGIAdapter> dxAdapter;
                        gekCheckResult(resultValue = dxDevice->GetParent(IID_PPV_ARGS(&dxAdapter)));
                        if (dxAdapter)
                        {
                            CComPtr<IDXGIFactory> dxFactory;
                            gekCheckResult(resultValue = dxAdapter->GetParent(IID_PPV_ARGS(&dxFactory)));
                            if (dxFactory)
                            {
                                gekCheckResult(resultValue = dxFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
                            }
                        }

                        gekCheckResult(resultValue = dxDevice->SetMaximumFrameLatency(1));

                        CComPtr<ID2D1Device> d2dDevice;
                        gekCheckResult(resultValue = d2dFactory->CreateDevice(dxDevice, &d2dDevice));
                        if (d2dDevice)
                        {
                            gekCheckResult(resultValue = d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDeviceContext));
                        }
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    gekCheckResult(resultValue = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&dwFactory)));
                }

                if (SUCCEEDED(resultValue))
                {
                    resultValue = createDefaultTargets();
                }
            }

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
            gekCheckScope(resultValue, fullScreen);
            if (!isChildWindow)
            {
                this->fullScreen = fullScreen;
                resultValue = dxSwapChain->SetFullscreenState(fullScreen, nullptr);
            }

            return resultValue;
        }

        STDMETHODIMP resize(void)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(d3dDeviceContext, E_FAIL);
            REQUIRE_RETURN(dxSwapChain, E_FAIL);

            gekCheckScope(resultValue);

            RECT clientRect;
            GetClientRect(this->window, &clientRect);
            this->width = (clientRect.right - clientRect.left);
            this->height = (clientRect.bottom - clientRect.top);

            d2dDeviceContext->SetTarget(nullptr);
            d3dDefaultRenderTargetView.Release();
            d3dDefaultDepthStencilView.Release();

            DXGI_MODE_DESC description;
            description.Width = width;
            description.Height = height;
            description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            description.RefreshRate.Numerator = 60;
            description.RefreshRate.Denominator = 1;
            description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            if (gekCheckResult(resultValue = dxSwapChain->ResizeTarget(&description)))
            {
                if (gekCheckResult(resultValue = dxSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0)))
                {
                    resultValue = createDefaultTargets();
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(VideoContext *) getDefaultContext(void)
        {
            return static_cast<VideoContext *>(this);
        }

        STDMETHODIMP_(OverlaySystem *) getOverlay(void)
        {
            return static_cast<OverlaySystem *>(this);
        }

        STDMETHODIMP_(UINT32) getWidth(void)
        {
            return width;
        }

        STDMETHODIMP_(UINT32) getHeight(void)
        {
            return height;
        }

        STDMETHODIMP_(bool) isFullScreen(void)
        {
            return fullScreen;
        }

        STDMETHODIMP createDeferredContext(VideoContext **returnObject)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);

            gekCheckScope(resultValue);
            CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
            gekCheckResult(resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext));
            if (d3dDeferredDeviceContext)
            {
                resultValue = E_OUTOFMEMORY;
                CComPtr<VideoContextImplementation> deferredContext(new VideoContextImplementation(d3dDeferredDeviceContext));
                if (deferredContext)
                {
                    gekCheckResult(resultValue = deferredContext->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }

            return resultValue;
        }

        STDMETHODIMP createEvent(IUnknown **returnObject)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue);

            D3D11_QUERY_DESC description;
            description.Query = D3D11_QUERY_EVENT;
            description.MiscFlags = 0;

            CComPtr<ID3D11Query> d3dQuery;
            gekCheckResult(resultValue = d3dDevice->CreateQuery(&description, &d3dQuery));
            if (d3dQuery)
            {
                gekCheckResult(resultValue = d3dQuery->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP_(void) setEvent(IUnknown *event)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            CComQIPtr<ID3D11Query> d3dQuery(event);
            d3dDeviceContext->End(d3dQuery);
        }

        STDMETHODIMP_(bool) isEventSet(IUnknown *event)
        {
            REQUIRE_RETURN(d3dDeviceContext, false);

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
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, renderStates.frontCounterClockwise,
                renderStates.depthBias,
                renderStates.depthBiasClamp,
                renderStates.slopeScaledDepthBias,
                renderStates.depthClipEnable,
                renderStates.scissorEnable,
                renderStates.multisampleEnable,
                renderStates.antialiasedLineEnable,
                UINT32(renderStates.fillMode),
                UINT32(renderStates.cullMode));

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
            gekCheckResult(resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates));
            if (d3dStates)
            {
                gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createDepthStates(IUnknown **returnObject, const Video::DepthStates &depthStates)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, depthStates.enable,
                UINT32(depthStates.comparisonFunction),
                depthStates.stencilEnable,
                depthStates.stencilReadMask,
                depthStates.stencilWriteMask,
                UINT32(depthStates.stencilFrontStates.failOperation),
                UINT32(depthStates.stencilFrontStates.depthFailOperation),
                UINT32(depthStates.stencilFrontStates.passOperation),
                UINT32(depthStates.stencilFrontStates.comparisonFunction),
                UINT32(depthStates.stencilBackStates.failOperation),
                UINT32(depthStates.stencilBackStates.depthFailOperation),
                UINT32(depthStates.stencilBackStates.passOperation),
                UINT32(depthStates.stencilBackStates.comparisonFunction),
                UINT32(depthStates.writeMask));

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
            gekCheckResult(resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates));
            if (d3dStates)
            {
                gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, blendStates.alphaToCoverage,
                blendStates.enable,
                UINT32(blendStates.colorSource),
                UINT32(blendStates.colorDestination),
                UINT32(blendStates.colorOperation),
                UINT32(blendStates.alphaSource),
                UINT32(blendStates.alphaDestination),
                UINT32(blendStates.alphaOperation),
                blendStates.writeMask);

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
            gekCheckResult(resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates));
            if (d3dStates)
            {
                gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createBlendStates(IUnknown **returnObject, const Video::IndependentBlendStates &blendStates)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue);

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
            gekCheckResult(resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates));
            if (d3dStates)
            {
                gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createSamplerStates(IUnknown **returnObject, const Video::SamplerStates &samplerStates)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, UINT32(samplerStates.addressModeU),
                UINT32(samplerStates.addressModeV),
                UINT32(samplerStates.addressModeW),
                samplerStates.mipLevelBias,
                samplerStates.maximumAnisotropy,
                UINT32(samplerStates.comparisonFunction),
                samplerStates.borderColor.r,
                samplerStates.borderColor.g,
                samplerStates.borderColor.b,
                samplerStates.borderColor.a,
                samplerStates.minimumMipLevel,
                samplerStates.maximumMipLevel,
                UINT32(samplerStates.filterMode));

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
            gekCheckResult(resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates));
            if (d3dStates)
            {
                gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createBuffer(VideoBuffer **returnObject, Video::Format format, UINT32 stride, UINT32 count, Video::BufferType type, UINT32 flags, LPCVOID data)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
            REQUIRE_RETURN(stride > 0, E_INVALIDARG);
            REQUIRE_RETURN(count > 0, E_INVALIDARG);

            gekCheckScope(resultValue, UINT32(format),
                stride,
                count,
                flags,
                LPCVOID(data));

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
                gekCheckResult(resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer));
            }
            else
            {
                D3D11_SUBRESOURCE_DATA resourceData;
                resourceData.pSysMem = data;
                resourceData.SysMemPitch = 0;
                resourceData.SysMemSlicePitch = 0;
                gekCheckResult(resultValue = d3dDevice->CreateBuffer(&bufferDescription, &resourceData, &d3dBuffer));
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
                    gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView));
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

                    gekCheckResult(resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView));
                    if (FAILED(resultValue))
                    {
                        return resultValue;
                    }
                }

                resultValue = E_OUTOFMEMORY;
                CComPtr<BufferImplementation> buffer(new BufferImplementation(format, stride, count, d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView));
                if (buffer)
                {
                    gekCheckResult(resultValue = buffer->QueryInterface(IID_PPV_ARGS(returnObject)));
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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(data);

            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->UpdateSubresource(d3dBuffer, 0, nullptr, data, 0, 0);
        }

        STDMETHODIMP mapBuffer(VideoBuffer *buffer, LPVOID *data, Video::Map mapping)
        {
            REQUIRE_RETURN(d3dDeviceContext, E_FAIL);
            REQUIRE_RETURN(data, E_INVALIDARG);

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
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(buffer);

            CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
            d3dDeviceContext->Unmap(d3dBuffer, 0);
        }

        STDMETHODIMP_(void) copyResource(IUnknown *destination, IUnknown *source)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(destination);
            REQUIRE_VOID_RETURN(source);

            CComQIPtr<ID3D11Resource> d3dDestination(destination);
            CComQIPtr<ID3D11Resource> d3dSource(source);
            d3dDeviceContext->CopyResource(d3dDestination, d3dSource);
        }

        STDMETHODIMP compileComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);
            REQUIRE_RETURN(programScript, E_INVALIDARG);
            REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

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
            gekCheckResult(resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "cs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors));
            if (d3dShaderBlob)
            {
                CComPtr<ID3D11ComputeShader> d3dShader;
                gekCheckResult(resultValue = d3dDevice->CreateComputeShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader));
                if (d3dShader)
                {
                    gekCheckResult(resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }
            else if (d3dCompilerErrors)
            {
                gekLogMessage(L"[error] %S", (LPCSTR)d3dCompilerErrors->GetBufferPointer());
            }

            return resultValue;
        }

        STDMETHODIMP compileVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Video::InputElement> *elementLayout, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);
            REQUIRE_RETURN(programScript, E_INVALIDARG);
            REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

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
            gekCheckResult(resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "vs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors));
            if (d3dShaderBlob)
            {
                CComPtr<ID3D11VertexShader> d3dShader;
                gekCheckResult(resultValue = d3dDevice->CreateVertexShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader));
                if (d3dShader)
                {
                    CComPtr<ID3D11InputLayout> d3dInputLayout;
                    if (elementLayout)
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
                            gekCheckResult(resultValue = d3dDevice->CreateInputLayout(inputElementList.data(), inputElementList.size(), d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), &d3dInputLayout));
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<VertexProgram> shader(new VertexProgram(d3dShader, d3dInputLayout));
                        if (shader)
                        {
                            gekCheckResult(resultValue = shader->QueryInterface(IID_PPV_ARGS(returnObject)));
                        }
                    }
                }
            }
            else if (d3dCompilerErrors)
            {
                gekLogMessage(L"[error] %S", (LPCSTR)d3dCompilerErrors->GetBufferPointer());
            }

            return resultValue;
        }

        STDMETHODIMP compileGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);
            REQUIRE_RETURN(programScript, E_INVALIDARG);
            REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

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
            gekCheckResult(resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "gs_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors));
            if (d3dShaderBlob)
            {
                CComPtr<ID3D11GeometryShader> d3dShader;
                gekCheckResult(resultValue = d3dDevice->CreateGeometryShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader));
                if (d3dShader)
                {
                    gekCheckResult(resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }
            else if (d3dCompilerErrors)
            {
                gekLogMessage(L"[error] %S", (LPCSTR)d3dCompilerErrors->GetBufferPointer());
            }

            return resultValue;
        }

        STDMETHODIMP compilePixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
        {
            REQUIRE_RETURN(d3dDevice, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);
            REQUIRE_RETURN(programScript, E_INVALIDARG);
            REQUIRE_RETURN(entryFunction, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

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
            gekCheckResult(resultValue = D3DCompile(programScript, (strlen(programScript) + 1), CW2A(fileName), d3dShaderMacroList.data(), includes, entryFunction, "ps_5_0", flags, 0, &d3dShaderBlob, &d3dCompilerErrors));
            if (d3dShaderBlob)
            {
                CComPtr<ID3D11PixelShader> d3dShader;
                gekCheckResult(resultValue = d3dDevice->CreatePixelShader(d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), nullptr, &d3dShader));
                if (d3dShader)
                {
                    gekCheckResult(resultValue = d3dShader->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }
            else if (d3dCompilerErrors)
            {
                gekLogMessage(L"[error] %S", (LPCSTR)d3dCompilerErrors->GetBufferPointer());
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
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

            CStringA progamScript;
            gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileComputeProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Video::InputElement> *elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

            CStringA progamScript;
            gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileVertexProgram(returnObject, fileName, progamScript, entryFunction, elementLayout, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

            CStringA progamScript;
            gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compileGeometryProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP loadPixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
        {
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, entryFunction);

            CStringA progamScript;
            gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
            if (SUCCEEDED(resultValue))
            {
                CComPtr<IncludeImplementation> include(new IncludeImplementation(fileName, onInclude));
                resultValue = compilePixelProgram(returnObject, fileName, progamScript, entryFunction, defineList, include);
            }

            return resultValue;
        }

        STDMETHODIMP createTexture(VideoTexture **returnObject, Video::Format format, UINT32 width, UINT32 height, UINT32 depth, UINT32 flags, UINT32 mipmaps)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, UINT32(format),
                width,
                height,
                depth,
                flags,
                mipmaps);

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
                gekCheckResult(resultValue = d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture2D));
                if (texture2D)
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
                gekCheckResult(resultValue = d3dDevice->CreateTexture3D(&textureDescription, nullptr, &texture2D));
                if (texture2D)
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

                    gekCheckResult(resultValue = d3dDevice->CreateRenderTargetView(d3dResource, &renderViewDescription, &d3dRenderTargetView));
                }

                CComPtr<ID3D11DepthStencilView> depthStencilView;
                if (SUCCEEDED(resultValue) && flags & Video::TextureFlags::DepthTarget)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = d3dFormat;
                    depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    gekCheckResult(resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &depthStencilView));
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (SUCCEEDED(resultValue) && flags & Video::TextureFlags::Resource)
                {
                    gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(d3dResource, nullptr, &d3dShaderResourceView));
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

                    gekCheckResult(resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView));
                }

                if (SUCCEEDED(resultValue))
                {
                    CComPtr<TextureImplementation> texture(new TextureImplementation(d3dResource, d3dRenderTargetView, depthStencilView, d3dShaderResourceView, d3dUnorderedAccessView, format, width, height, depth));
                    if (texture)
                    {
                        gekCheckResult(resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadTexture(VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName, flags);

            std::vector<UINT8> fileData;
            gekCheckResult(resultValue = Gek::FileSystem::load(fileName, fileData));
            if (SUCCEEDED(resultValue))
            {
                ::DirectX::ScratchImage scratchImage;
                ::DirectX::TexMetadata textureMetaData;

                CPathW filePath(fileName);
                CStringW extension(filePath.GetExtension());
                if (extension.CompareNoCase(L".dds") == 0)
                {
                    gekCheckResult(resultValue = ::DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, &textureMetaData, scratchImage));
                }
                else if(extension.CompareNoCase(L".tga") == 0)
                {
                    gekCheckResult(resultValue = ::DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), &textureMetaData, scratchImage));
                }
                else if (extension.CompareNoCase(L".png") == 0)
                {
                    gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_PNG, &textureMetaData, scratchImage));
                }
                else if (extension.CompareNoCase(L".bmp") == 0)
                {
                    gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_BMP, &textureMetaData, scratchImage));
                }
                else if (extension.CompareNoCase(L".jpg") == 0 ||
                         extension.CompareNoCase(L".jpeg") == 0)
                {
                    gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_JPEG, &textureMetaData, scratchImage));
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
                    gekCheckResult(resultValue = ::DirectX::CreateShaderResourceView(d3dDevice, scratchImage.GetImages(), scratchImage.GetImageCount(), scratchImage.GetMetadata(), &d3dShaderResourceView));
                    if (d3dShaderResourceView)
                    {
                        resultValue = E_UNEXPECTED;
                        CComPtr<ID3D11Resource> d3dResource;
                        d3dShaderResourceView->GetResource(&d3dResource);
                        if (d3dResource)
                        {
                            resultValue = E_OUTOFMEMORY;
                            CComPtr<TextureImplementation> texture(new TextureImplementation(d3dResource, nullptr, nullptr, d3dShaderResourceView, nullptr, Video::Format::Unknown, scratchImage.GetMetadata().width, scratchImage.GetMetadata().height, scratchImage.GetMetadata().depth));
                            if (texture)
                            {
                                gekCheckResult(resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject)));
                            }
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP loadCubeMap(VideoTexture **returnObject, LPCWSTR fileNameList[6], UINT32 flags)
        {
            REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

            gekCheckScope(resultValue, fileNameList[0],
                fileNameList[1], 
                fileNameList[2], 
                fileNameList[3], 
                fileNameList[4], 
                fileNameList[5], 
                flags);

            ::DirectX::ScratchImage cubeMapList[6];
            ::DirectX::TexMetadata cubeMapMetaData;
            for (UINT32 side = 0; side < 6; side++)
            {
                std::vector<UINT8> fileData;
                gekCheckResult(resultValue = Gek::FileSystem::load(fileNameList[side], fileData));
                if (SUCCEEDED(resultValue))
                {
                    CPathW filePath(fileNameList[side]);
                    CStringW extension(filePath.GetExtension());
                    if (extension.CompareNoCase(L".dds") == 0)
                    {
                        gekCheckResult(resultValue = ::DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, &cubeMapMetaData, cubeMapList[side]));
                    }
                    else if (extension.CompareNoCase(L".tga") == 0)
                    {
                        gekCheckResult(resultValue = ::DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), &cubeMapMetaData, cubeMapList[side]));
                    }
                    else if (extension.CompareNoCase(L".png") == 0)
                    {
                        gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_PNG, &cubeMapMetaData, cubeMapList[side]));
                    }
                    else if (extension.CompareNoCase(L".bmp") == 0)
                    {
                        gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_BMP, &cubeMapMetaData, cubeMapList[side]));
                    }
                    else if (extension.CompareNoCase(L".jpg") == 0 ||
                        extension.CompareNoCase(L".jpeg") == 0)
                    {
                        gekCheckResult(resultValue = ::DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), ::DirectX::WIC_CODEC_JPEG, &cubeMapMetaData, cubeMapList[side]));
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
                gekCheckResult(resultValue = ::DirectX::CreateShaderResourceView(d3dDevice, cubeMap.GetImages(), cubeMap.GetImageCount(), cubeMap.GetMetadata(), &d3dShaderResourceView));
                if (d3dShaderResourceView)
                {
                    resultValue = E_UNEXPECTED;
                    CComPtr<ID3D11Resource> d3dResource;
                    d3dShaderResourceView->GetResource(&d3dResource);
                    if (d3dResource)
                    {
                        resultValue = E_OUTOFMEMORY;
                        CComPtr<TextureImplementation> texture(new TextureImplementation(d3dResource, nullptr, nullptr, d3dShaderResourceView, nullptr, Video::Format::Unknown, cubeMapMetaData.width, cubeMapMetaData.height, 1));
                        if (texture)
                        {
                            gekCheckResult(resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject)));
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(void) updateTexture(VideoTexture *texture, LPCVOID data, UINT32 pitch, Shapes::Rectangle<UINT32> *destinationRectangle)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);

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

        STDMETHODIMP_(void) clearDefaultRenderTarget(const Math::Color &colorClear)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(d3dDefaultRenderTargetView);
            d3dDeviceContext->ClearRenderTargetView(d3dDefaultRenderTargetView, colorClear.data);
        }

        STDMETHODIMP_(void) clearDefaultDepthStencilTarget(UINT32 flags, float depthClear, UINT32 stencilClear)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(d3dDefaultDepthStencilView);
            d3dDeviceContext->ClearDepthStencilView(d3dDefaultDepthStencilView,
                ((flags & Video::ClearMask::Depth ? D3D11_CLEAR_DEPTH : 0) |
                    (flags & Video::ClearMask::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                depthClear, stencilClear);
        }

        STDMETHODIMP_(void) setDefaultTargets(VideoContext *context, IUnknown *depthBuffer)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext || context);
            REQUIRE_VOID_RETURN(d3dDefaultRenderTargetView);

            D3D11_VIEWPORT viewPortList;
            viewPortList.TopLeftX = 0.0f;
            viewPortList.TopLeftY = 0.0f;
            viewPortList.Width = float(width);
            viewPortList.Height = float(height);
            viewPortList.MinDepth = 0.0f;
            viewPortList.MaxDepth = 1.0f;

            CComQIPtr<ID3D11DeviceContext> d3dDeferredContext(context);
            if (d3dDeferredContext)
            {
                d3dDeferredContext->OMSetRenderTargets(1, &d3dDefaultRenderTargetView.p, d3dDefaultDepthStencilView);
                d3dDeferredContext->RSSetViewports(1, &viewPortList);
            }
            else
            {
                d3dDeviceContext->OMSetRenderTargets(1, &d3dDefaultRenderTargetView.p, d3dDefaultDepthStencilView);
                d3dDeviceContext->RSSetViewports(1, &viewPortList);
            }
        }

        STDMETHODIMP_(void) executeCommandList(IUnknown *commandList)
        {
            REQUIRE_VOID_RETURN(d3dDeviceContext);
            REQUIRE_VOID_RETURN(commandList);

            CComQIPtr<ID3D11CommandList> d3dCommandList(commandList);
            d3dDeviceContext->ExecuteCommandList(d3dCommandList, FALSE);
        }

        STDMETHODIMP_(void) present(bool waitForVerticalSync)
        {
            REQUIRE_VOID_RETURN(dxSwapChain);

            dxSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
        }

        // OverlaySystem
        STDMETHODIMP createBrush(IUnknown **returnObject, const Math::Color &color)
        {
            REQUIRE_RETURN(d2dDeviceContext, E_INVALIDARG);

            gekCheckScope(resultValue, color.r, color.g, color.b, color.a);

            CComPtr<ID2D1SolidColorBrush> d2dSolidBrush;
            gekCheckResult(resultValue = d2dDeviceContext->CreateSolidColorBrush(*(D2D1_COLOR_F *)&color, &d2dSolidBrush));
            if (d2dSolidBrush)
            {
                gekCheckResult(resultValue = d2dSolidBrush->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP createBrush(IUnknown **returnObject, const std::vector<Video::GradientPoint> &stopPoints, const Shapes::Rectangle<float> &extents)
        {
            REQUIRE_RETURN(d2dDeviceContext, E_INVALIDARG);

            gekCheckScope(resultValue);

            CComPtr<ID2D1GradientStopCollection> spStops;
            gekCheckResult(resultValue = d2dDeviceContext->CreateGradientStopCollection((D2D1_GRADIENT_STOP *)stopPoints.data(), stopPoints.size(), &spStops));
            if (spStops)
            {
                CComPtr<ID2D1LinearGradientBrush> d2dGradientBrush;
                gekCheckResult(resultValue = d2dDeviceContext->CreateLinearGradientBrush(*(D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *)&extents, spStops, &d2dGradientBrush));
                if (d2dGradientBrush)
                {
                    gekCheckResult(resultValue = d2dGradientBrush->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }

            return resultValue;
        }

        STDMETHODIMP createFont(IUnknown **returnObject, LPCWSTR face, UINT32 weight, Video::FontStyle style, float size)
        {
            REQUIRE_RETURN(d2dDeviceContext, E_FAIL);
            REQUIRE_RETURN(face, E_INVALIDARG);

            gekCheckScope(resultValue, face,
                weight,
                UINT32(style),
                size);

            CComPtr<IDWriteTextFormat> dwTextFormat;
            gekCheckResult(resultValue = dwFactory->CreateTextFormat(face, nullptr, DWRITE_FONT_WEIGHT(weight), DirectX::FontStyleList[static_cast<UINT8>(style)], DWRITE_FONT_STRETCH_NORMAL, size, L"", &dwTextFormat));
            if (dwTextFormat)
            {
                gekCheckResult(resultValue = dwTextFormat->QueryInterface(IID_PPV_ARGS(returnObject)));
            }

            return resultValue;
        }

        STDMETHODIMP loadBitmap(IUnknown **returnObject, LPCWSTR fileName)
        {
            REQUIRE_RETURN(d2dDeviceContext, E_FAIL);
            REQUIRE_RETURN(fileName, E_INVALIDARG);

            gekCheckScope(resultValue, fileName);

            CComPtr<IWICImagingFactory> imagingFactory;
            gekCheckResult(resultValue = imagingFactory.CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER));
            if (imagingFactory)
            {
                CComPtr<IWICBitmapDecoder> bitmapDecoder;
                gekCheckResult(resultValue = imagingFactory->CreateDecoderFromFilename(Gek::FileSystem::expandPath(fileName), NULL, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &bitmapDecoder));
                if (bitmapDecoder)
                {
                    CComPtr<IWICBitmapFrameDecode> firstSourceFrame;
                    gekCheckResult(resultValue = bitmapDecoder->GetFrame(0, &firstSourceFrame));
                    if (firstSourceFrame)
                    {
                        CComPtr<IWICFormatConverter> formatConverter;
                        gekCheckResult(resultValue = imagingFactory->CreateFormatConverter(&formatConverter));
                        if (formatConverter)
                        {
                            if (SUCCEEDED(formatConverter->Initialize(firstSourceFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeMedianCut)))
                            {
                                CComPtr<ID2D1Bitmap1> bitmap;
                                gekCheckResult(resultValue = d2dDeviceContext->CreateBitmapFromWicBitmap(formatConverter, NULL, &bitmap));
                                if (bitmap)
                                {
                                    gekCheckResult(resultValue = bitmap->QueryInterface(IID_PPV_ARGS(returnObject)));
                                }
                            }
                        }
                    }
                }
            }

            return resultValue;
        }

        STDMETHODIMP createGeometry(OverlayGeometry **returnObject)
        {
            REQUIRE_RETURN(dwFactory, E_FAIL);
            REQUIRE_RETURN(returnObject, E_INVALIDARG);

            gekCheckScope(resultValue);

            CComPtr<ID2D1PathGeometry> d2dPathGeometry;
            gekCheckResult(resultValue = d2dFactory->CreatePathGeometry(&d2dPathGeometry));
            if (d2dPathGeometry)
            {
                resultValue = E_OUTOFMEMORY;
                CComPtr<GeometryImplementation> geometry(new GeometryImplementation(d2dPathGeometry));
                if (geometry)
                {
                    gekCheckResult(resultValue = geometry->QueryInterface(IID_PPV_ARGS(returnObject)));
                }
            }

            return resultValue;
        }

        STDMETHODIMP_(void) setTransform(const Math::Float3x2 &matrix)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            d2dDeviceContext->SetTransform(*(D2D1_MATRIX_3X2_F *)&matrix);
        }

        STDMETHODIMP_(void) drawText(const Shapes::Rectangle<float> &extents, IUnknown *font, IUnknown *brush, LPCWSTR format, ...)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            REQUIRE_VOID_RETURN(format);

            CComQIPtr<ID2D1Brush> d2dBrush(brush);
            CComQIPtr<IDWriteTextFormat> dwTextFormat(font);
            if (d2dBrush && dwTextFormat)
            {
                CStringW text;
                va_list variableList;
                va_start(variableList, format);
                text.AppendFormatV(format, variableList);
                va_end(variableList);
                if (!text.IsEmpty())
                {
                    d2dDeviceContext->DrawText(text, text.GetLength(), dwTextFormat, *(D2D1_RECT_F *)&extents, d2dBrush);
                }
            }
        }

        STDMETHODIMP_(void) drawRectangle(const Shapes::Rectangle<float> &extents, IUnknown *brush, bool fillShape)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            CComQIPtr<ID2D1Brush> d2dBrush(brush);
            if (d2dBrush)
            {
                if (fillShape)
                {
                    d2dDeviceContext->FillRectangle(*(D2D1_RECT_F *)&extents, d2dBrush);
                }
                else
                {
                    d2dDeviceContext->DrawRectangle(*(D2D1_RECT_F *)&extents, d2dBrush);
                }
            }
        }

        STDMETHODIMP_(void) drawRectangle(const Shapes::Rectangle<float> &extents, const Math::Float2 &cornerRadius, IUnknown *brush, bool fillShape)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            CComQIPtr<ID2D1Brush> d2dBrush(brush);
            if (d2dBrush)
            {
                if (fillShape)
                {
                    d2dDeviceContext->FillRoundedRectangle({ *(D2D1_RECT_F *)&extents, cornerRadius.x, cornerRadius.y }, d2dBrush);
                }
                else
                {
                    d2dDeviceContext->DrawRoundedRectangle({ *(D2D1_RECT_F *)&extents, cornerRadius.x, cornerRadius.y }, d2dBrush);
                }
            }
        }

        STDMETHODIMP_(void) drawBitmap(IUnknown *bitmap, const Shapes::Rectangle<float> &destinationExtents, Video::InterpolationMode interpolationMode, float opacity)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            CComQIPtr<ID2D1Bitmap1> d2dBitmap(bitmap);
            if (d2dBitmap)
            {
                d2dDeviceContext->DrawBitmap(d2dBitmap, (const D2D1_RECT_F *)&destinationExtents, opacity, DirectX::InterpolationMode[static_cast<UINT8>(interpolationMode)]);
            }
        }

        STDMETHODIMP_(void) drawBitmap(IUnknown *bitmap, const Shapes::Rectangle<float> &destinationExtents, const Shapes::Rectangle<float> &sourceExtents, Video::InterpolationMode interpolationMode, float opacity)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            CComQIPtr<ID2D1Bitmap1> d2dBitmap(bitmap);
            if (d2dBitmap)
            {
                d2dDeviceContext->DrawBitmap(d2dBitmap, (const D2D1_RECT_F *)&destinationExtents, opacity, DirectX::InterpolationMode[static_cast<UINT8>(interpolationMode)], (const D2D1_RECT_F *)&sourceExtents);
            }
        }

        STDMETHODIMP_(void) drawGeometry(OverlayGeometry *geometry, IUnknown *brush, bool fillShape)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            REQUIRE_VOID_RETURN(geometry);

            CComQIPtr<ID2D1PathGeometry> d2dPathGeometry(geometry);
            CComQIPtr<ID2D1Brush> d2dBrush(brush);
            if (d2dPathGeometry && d2dBrush)
            {
                if (fillShape)
                {
                    d2dDeviceContext->FillGeometry(d2dPathGeometry, d2dBrush);
                }
                else
                {
                    d2dDeviceContext->DrawGeometry(d2dPathGeometry, d2dBrush);
                }
            }
        }

        STDMETHODIMP_(void) beginDraw(void)
        {
            REQUIRE_VOID_RETURN(d2dDeviceContext);
            d2dDeviceContext->BeginDraw();
        }

        STDMETHODIMP endDraw(void)
        {
            REQUIRE_RETURN(d2dDeviceContext, E_FAIL);
            return d2dDeviceContext->EndDraw();
        }
    };

    REGISTER_CLASS(VideoSystemImplementation);
}; // namespace Gek
