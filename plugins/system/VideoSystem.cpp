#pragma warning(disable : 4005)

#include "GEK\Context\Common.h"
#include "GEK\Context\BaseUnknown.h"
#include "GEK\Context\BaseUser.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\System\VideoInterface.h"
#include <atlbase.h>
#include <atlpath.h>
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <algorithm>
#include <memory>
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <wincodec.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Gek
{
    using namespace Video2D;
    using namespace Video3D;
    namespace Video
    {        
        static const DXGI_FORMAT d3dFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_R8_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R16_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_D16_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            DXGI_FORMAT_D32_FLOAT,
        };

        static const UINT32 d3dFormatStrideList[] = 
        {
            0,
            sizeof(UINT8),
            (sizeof(UINT8) * 2),
            (sizeof(UINT8) * 4),
            (sizeof(UINT8) * 4),
            sizeof(UINT16),
            (sizeof(UINT16) * 2),
            (sizeof(UINT16) * 4),
            sizeof(UINT32),
            (sizeof(UINT32) * 2),
            (sizeof(UINT32) * 3),
            (sizeof(UINT32) * 4),
            sizeof(float),
            (sizeof(float) * 2),
            (sizeof(float) * 3),
            (sizeof(float) * 4),
            (sizeof(float) / 2),
            sizeof(float),
            (sizeof(float) * 2),
            sizeof(UINT16),
            sizeof(UINT32),
            (sizeof(float) * 4),
        };

        static const D3D11_DEPTH_WRITE_MASK d3dDepthWriteMaskList[] =
        {
            D3D11_DEPTH_WRITE_MASK_ZERO,
            D3D11_DEPTH_WRITE_MASK_ALL,
        };

        static const D3D11_TEXTURE_ADDRESS_MODE d3dAddressModeList[] = 
        {
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
            D3D11_TEXTURE_ADDRESS_BORDER,
        };

        static const D3D11_COMPARISON_FUNC d3dComparisonFunctionList[] = 
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

        static const D3D11_STENCIL_OP d3dStencilOperationList[] = 
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

        static const D3D11_BLEND d3dBlendSourceList[] =
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

        static const D3D11_BLEND_OP d3dBlendOperationList[] = 
        {
            D3D11_BLEND_OP_ADD,
            D3D11_BLEND_OP_SUBTRACT,
            D3D11_BLEND_OP_REV_SUBTRACT,
            D3D11_BLEND_OP_MIN,
            D3D11_BLEND_OP_MAX,
        };

        static const D3D11_FILL_MODE d3dFillModeList[] =
        {
            D3D11_FILL_WIREFRAME,
            D3D11_FILL_SOLID,
        };

        static const D3D11_CULL_MODE d3dCullModeList[] =
        {
            D3D11_CULL_NONE,
            D3D11_CULL_FRONT,
            D3D11_CULL_BACK,
        };

        static const D3D11_FILTER d3dFilterList[] =
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

        static const D3D11_PRIMITIVE_TOPOLOGY d3dTopologList[] = 
        {
            D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

        static const DWRITE_FONT_STYLE d2dFontStyleList[] =
        {
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STYLE_ITALIC,
        };

        static const D2D1_INTERPOLATION_MODE d2dInterpolationMode[] =
        {
            D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            D2D1_INTERPOLATION_MODE_LINEAR,
            D2D1_INTERPOLATION_MODE_CUBIC,
            D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR,
            D2D1_INTERPOLATION_MODE_ANISOTROPIC,
            D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,
        };

        class VertexProgram : public BaseUnknown
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

        class Buffer : public BaseUnknown
                     , public BufferInterface
        {
        private:
            CComPtr<ID3D11Buffer> d3dBuffer;
            CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
            CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
            Format format;
            UINT32 stride;
            UINT32 count;

        public:
            Buffer(UINT32 stride, UINT32 count, ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView = nullptr, ID3D11UnorderedAccessView *d3dUnorderedAccessView = nullptr)
                : format(Format::UNKNOWN)
                , stride(stride)
                , count(count)
                , d3dBuffer(d3dBuffer)
                , d3dShaderResourceView(d3dShaderResourceView)
                , d3dUnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            Buffer(Format format, UINT32 count, ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView = nullptr, ID3D11UnorderedAccessView *d3dUnorderedAccessView = nullptr)
                : format(format)
                , stride(0)
                , count(count)
                , d3dBuffer(d3dBuffer)
                , d3dShaderResourceView(d3dShaderResourceView)
                , d3dUnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            BEGIN_INTERFACE_LIST(Buffer)
                INTERFACE_LIST_ENTRY_COM(BufferInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Buffer, d3dBuffer)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, d3dShaderResourceView)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, d3dUnorderedAccessView)
            END_INTERFACE_LIST_UNKNOWN

            // BufferInterface
            STDMETHODIMP_(Format) getFormat(void)
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

        class Texture : public BaseUnknown
                      , public TextureInterface
        {
        protected:
            CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
            CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
            UINT32 width;
            UINT32 height;
            UINT32 depth;

        public:
            Texture(UINT32 width, UINT32 height, UINT32 depth, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView)
                : width(width)
                , height(height)
                , depth(depth)
                , d3dShaderResourceView(d3dShaderResourceView)
                , d3dUnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            BEGIN_INTERFACE_LIST(Texture)
                INTERFACE_LIST_ENTRY_COM(TextureInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, d3dShaderResourceView)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, d3dUnorderedAccessView)
            END_INTERFACE_LIST_UNKNOWN

            // TextureInterface
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
        };

        class RenderTarget : public Texture
        {
        private:
            CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;

        public:
            RenderTarget(UINT32 width, UINT32 height, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11RenderTargetView *d3dRenderTargetView)
                : Texture(width, height, 0, d3dShaderResourceView, nullptr)
                , d3dRenderTargetView(d3dRenderTargetView)
            {
            }

            BEGIN_INTERFACE_LIST(RenderTarget)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, d3dRenderTargetView)
            END_INTERFACE_LIST_BASE(Texture)
        };

        class Geometry : public BaseUnknown
                       , public GeometryInterface
        {
        private:
            CComPtr<ID2D1PathGeometry> d2dPathGeometry;
            CComPtr<ID2D1GeometrySink> d2dGeometrySink;
            bool figureHasBegun;

        public:
            Geometry(ID2D1PathGeometry *pGeometry)
                : d2dPathGeometry(d2dPathGeometry)
                , figureHasBegun(false)
            {
            }

            BEGIN_INTERFACE_LIST(Geometry)
                INTERFACE_LIST_ENTRY_COM(GeometryInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID2D1PathGeometry, d2dPathGeometry)
            END_INTERFACE_LIST_UNKNOWN

            // GeometryInterface
            STDMETHODIMP openShape(void)
            {
                REQUIRE_RETURN(d2dPathGeometry, E_FAIL);

                d2dGeometrySink = nullptr;
                return d2dPathGeometry->Open(&d2dGeometrySink);
            }

            STDMETHODIMP closeShape(void)
            {
                REQUIRE_RETURN(d2dGeometrySink, E_FAIL);

                HRESULT resultValue = d2dGeometrySink->Close();
                if (SUCCEEDED(resultValue))
                {
                    d2dGeometrySink = nullptr;
                }

                return resultValue;
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

            STDMETHODIMP createWidened(float width, float tolerance, GeometryInterface **returnObject)
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
                                CComPtr<Geometry> geometry(new Geometry(d2dWidePathGeometry));
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

        class Include : public BaseUnknown
                      , public ID3DInclude
        {
        private:
            CPathW shaderFilePath;
            std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude;
            std::vector<UINT8> includeBuffer;

        public:
            Include(const CStringW &shaderFileName, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude)
                : shaderFilePath(shaderFileName)
                , onInclude(onInclude)
            {
                shaderFilePath.RemoveFileSpec();
            }

            BEGIN_INTERFACE_LIST(Include)
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

        class Context : public Gek::Context::BaseUser
                      , public Video3D::ContextInterface
        {
            friend class System;

        protected:
            class System
            {
            protected:
                ID3D11DeviceContext *d3dDeviceContext;

            public:
                System(ID3D11DeviceContext *d3dDeviceContext)
                    : d3dDeviceContext(d3dDeviceContext)
                {
                }
            };

            class ComputeSystem : public System
                                , public SubSystemInterface
            {
            public:
                ComputeSystem(ID3D11DeviceContext *d3dDeviceContext)
                    : System(d3dDeviceContext)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(IUnknown *program)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11ComputeShader> d3dComputeShader(program);
                    if (d3dComputeShader)
                    {
                        d3dDeviceContext->CSSetShader(d3dComputeShader, nullptr, 0);
                    }
                    else
                    {
                        d3dDeviceContext->CSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(BufferInterface *buffer, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                    if (d3dBuffer)
                    {
                        ID3D11Buffer *d3dBufferList[1] = { d3dBuffer };
                        d3dDeviceContext->CSSetConstantBuffers(stage, 1, d3dBufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
                    if (d3dSamplerState)
                    {
                        ID3D11SamplerState *d3dSamplerStateList[1] = { d3dSamplerState };
                        d3dDeviceContext->CSSetSamplers(stage, 1, d3dSamplerStateList);
                    }
                }

                STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    ID3D11ShaderResourceView *d3dShaderResourceViewList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
                    if (d3dShaderResourceView)
                    {
                        d3dShaderResourceViewList[0] = d3dShaderResourceView;
                    }

                    d3dDeviceContext->CSSetShaderResources(stage, 1, d3dShaderResourceViewList);
                }

                STDMETHODIMP_(void) setUnorderedAccess(IUnknown *unorderedAccess, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    ID3D11UnorderedAccessView *d3dUnorderedAccessViewList[1] = { nullptr };
                    CComQIPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView(unorderedAccess);
                    if (d3dUnorderedAccessView)
                    {
                        d3dUnorderedAccessViewList[0] = d3dUnorderedAccessView;
                    }

                    d3dDeviceContext->CSSetUnorderedAccessViews(stage, 1, d3dUnorderedAccessViewList, nullptr);
                }
            };

            class VertexSystem : public System
                               , public SubSystemInterface
            {
            public:
                VertexSystem(ID3D11DeviceContext *d3dDeviceContext)
                    : System(d3dDeviceContext)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(IUnknown *program)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11VertexShader> d3dVertexShader(program);
                    CComQIPtr<ID3D11InputLayout> d3dInputLayout(program);
                    if (d3dVertexShader && d3dInputLayout)
                    {
                        d3dDeviceContext->VSSetShader(d3dVertexShader, nullptr, 0);
                        d3dDeviceContext->IASetInputLayout(d3dInputLayout);
                    }
                    else
                    {
                        d3dDeviceContext->VSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(BufferInterface *buffer, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                    if (d3dBuffer)
                    {
                        ID3D11Buffer *d3dBufferList[1] = { d3dBuffer };
                        d3dDeviceContext->VSSetConstantBuffers(stage, 1, d3dBufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
                    if (d3dSamplerState)
                    {
                        ID3D11SamplerState *d3dSamplerStateList[1] = { d3dSamplerState };
                        d3dDeviceContext->VSSetSamplers(stage, 1, d3dSamplerStateList);
                    }
                }

                STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    ID3D11ShaderResourceView *d3dShaderResourceViewList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
                    if (d3dShaderResourceView)
                    {
                        d3dShaderResourceViewList[0] = d3dShaderResourceView;
                    }

                    d3dDeviceContext->VSSetShaderResources(stage, 1, d3dShaderResourceViewList);
                }
            };

            class GeometrySystem : public System
                                 , public SubSystemInterface
            {
            public:
                GeometrySystem(ID3D11DeviceContext *d3dDeviceContext)
                    : System(d3dDeviceContext)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(IUnknown *program)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11GeometryShader> d3dGeometryShader(program);
                    if (d3dGeometryShader)
                    {
                        d3dDeviceContext->GSSetShader(d3dGeometryShader, nullptr, 0);
                    }
                    else
                    {
                        d3dDeviceContext->GSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(BufferInterface *buffer, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                    if (d3dBuffer)
                    {
                        ID3D11Buffer *d3dBufferList[1] = { d3dBuffer };
                        d3dDeviceContext->GSSetConstantBuffers(stage, 1, d3dBufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
                    if (d3dSamplerState)
                    {
                        ID3D11SamplerState *d3dSamplerStateList[1] = { d3dSamplerState };
                        d3dDeviceContext->GSSetSamplers(stage, 1, d3dSamplerStateList);
                    }
                }

                STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    ID3D11ShaderResourceView *d3dShaderResourceViewList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
                    if (d3dShaderResourceView)
                    {
                        d3dShaderResourceViewList[0] = d3dShaderResourceView;
                    }

                    d3dDeviceContext->GSSetShaderResources(stage, 1, d3dShaderResourceViewList);
                }
            };

            class PixelSystem : public System
                              , public SubSystemInterface
            {
            public:
                PixelSystem(ID3D11DeviceContext *d3dDeviceContext)
                    : System(d3dDeviceContext)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(IUnknown *program)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11PixelShader> d3dPixelShader(program);
                    if (d3dPixelShader)
                    {
                        d3dDeviceContext->PSSetShader(d3dPixelShader, nullptr, 0);
                    }
                    else
                    {
                        d3dDeviceContext->PSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(BufferInterface *buffer, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                    if (d3dBuffer)
                    {
                        ID3D11Buffer *d3dBufferList[1] = { d3dBuffer };
                        d3dDeviceContext->PSSetConstantBuffers(stage, 1, d3dBufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(IUnknown *samplerStates, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    CComQIPtr<ID3D11SamplerState> d3dSamplerState(samplerStates);
                    if (d3dSamplerState)
                    {
                        ID3D11SamplerState *d3dSamplerStateList[1] = { d3dSamplerState };
                        d3dDeviceContext->PSSetSamplers(stage, 1, d3dSamplerStateList);
                    }
                }

                STDMETHODIMP_(void) setResource(IUnknown *resource, UINT32 stage)
                {
                    REQUIRE_VOID_RETURN(d3dDeviceContext);
                    ID3D11ShaderResourceView *d3dShaderResourceViewList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> d3dShaderResourceView(resource);
                    if (d3dShaderResourceView)
                    {
                        d3dShaderResourceViewList[0] = d3dShaderResourceView;
                    }

                    d3dDeviceContext->PSSetShaderResources(stage, 1, d3dShaderResourceViewList);
                }
            };

        protected:
            CComPtr<ID3D11DeviceContext> d3dDeviceContext;
            std::unique_ptr<SubSystemInterface> computeSystem;
            std::unique_ptr<SubSystemInterface> vertexSystem;
            std::unique_ptr<SubSystemInterface> geomtrySystem;
            std::unique_ptr<SubSystemInterface> pixelSystem;

        public:
            Context(void)
            {
            }

            Context(ID3D11DeviceContext *d3dDeviceContext)
                : d3dDeviceContext(d3dDeviceContext)
                , computeSystem(new ComputeSystem(d3dDeviceContext))
                , vertexSystem(new VertexSystem(d3dDeviceContext))
                , geomtrySystem(new GeometrySystem(d3dDeviceContext))
                , pixelSystem(new PixelSystem(d3dDeviceContext))
            {
            }

            BEGIN_INTERFACE_LIST(Context)
                INTERFACE_LIST_ENTRY_COM(ContextInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DeviceContext, d3dDeviceContext)
            END_INTERFACE_LIST_USER

            // ContextInterface
            STDMETHODIMP_(SubSystemInterface *) getComputeSystem(void)
            {
                REQUIRE_RETURN(computeSystem, nullptr);
                return computeSystem.get();
            }

            STDMETHODIMP_(SubSystemInterface *) getVertexSystem(void)
            {
                REQUIRE_RETURN(vertexSystem, nullptr);
                return vertexSystem.get();
            }

            STDMETHODIMP_(SubSystemInterface *) getGeometrySystem(void)
            {
                REQUIRE_RETURN(geomtrySystem, nullptr);
                return geomtrySystem.get();
            }

            STDMETHODIMP_(SubSystemInterface *) getPixelSystem(void)
            {
                REQUIRE_RETURN(pixelSystem, nullptr);
                return pixelSystem.get();
            }

            STDMETHODIMP_(void) clearResources(void)
            {
                static ID3D11ShaderResourceView *const d3dShaderResourceViewList[] =
                {
                    nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr,
                };

                static ID3D11RenderTargetView  *const d3dRenderTargetViewList[] =
                {
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                };

                REQUIRE_VOID_RETURN(d3dDeviceContext);
                d3dDeviceContext->CSSetShaderResources(0, 10, d3dShaderResourceViewList);
                d3dDeviceContext->VSSetShaderResources(0, 10, d3dShaderResourceViewList);
                d3dDeviceContext->GSSetShaderResources(0, 10, d3dShaderResourceViewList);
                d3dDeviceContext->PSSetShaderResources(0, 10, d3dShaderResourceViewList);
                d3dDeviceContext->OMSetRenderTargets(6, d3dRenderTargetViewList, nullptr);
            }

            STDMETHODIMP_(void) setViewports(const std::vector<Gek::Video3D::ViewPort> &viewPortList)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext && viewPortList.size() > 0);
                d3dDeviceContext->RSSetViewports(viewPortList.size(), (D3D11_VIEWPORT *)viewPortList.data());
            }

            STDMETHODIMP_(void) setScissorRect(const std::vector<Shape::Rectangle<UINT32>> &rectangleList)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext && rectangleList.size() > 0);
                d3dDeviceContext->RSSetScissorRects(rectangleList.size(), (D3D11_RECT *)rectangleList.data());
            }

            STDMETHODIMP_(void) clearRenderTarget(TextureInterface *renderTarget, const Math::Float4 &colorClear)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11RenderTargetView> d3dRenderTargetView(renderTarget);
                if (d3dRenderTargetView)
                {
                    d3dDeviceContext->ClearRenderTargetView(d3dRenderTargetView, colorClear.rgba);
                }
            }

            STDMETHODIMP_(void) clearDepthStencilTarget(IUnknown *depthBuffer, UINT32 flags, float depthClear, UINT32 stencilClear)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11DepthStencilView> d3dDepthStencilView(depthBuffer);
                if (d3dDepthStencilView)
                {
                    d3dDeviceContext->ClearDepthStencilView(d3dDepthStencilView,
                        ((flags & Gek::Video3D::ClearMask::DEPTH ? D3D11_CLEAR_DEPTH : 0) |
                         (flags & Gek::Video3D::ClearMask::STENCIL ? D3D11_CLEAR_STENCIL : 0)),
                          depthClear, stencilClear);
                }
            }

            STDMETHODIMP_(void) setRenderTargets(const std::vector<TextureInterface *> &renderTargetList, IUnknown *depthBuffer)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                std::vector<ID3D11RenderTargetView *> d3dRenderTargetViewList;
                for (auto &renderTarget : renderTargetList)
                {
                    CComQIPtr<ID3D11RenderTargetView> d3dRenderTargetView(renderTarget);
                    d3dRenderTargetViewList.push_back(d3dRenderTargetView);
                }

                CComQIPtr<ID3D11DepthStencilView> d3dDepthStencilView(depthBuffer);
                if (d3dDepthStencilView)
                {
                    d3dDeviceContext->OMSetRenderTargets(d3dRenderTargetViewList.size(), d3dRenderTargetViewList.data(), d3dDepthStencilView);
                }
                else
                {
                    d3dDeviceContext->OMSetRenderTargets(d3dRenderTargetViewList.size(), d3dRenderTargetViewList.data(), nullptr);
                }
            }

            STDMETHODIMP_(void) setRenderStates(IUnknown *renderStates)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11RasterizerState> d3dRasterizerState(renderStates);
                if (d3dRasterizerState)
                {
                    d3dDeviceContext->RSSetState(d3dRasterizerState);
                }
            }

            STDMETHODIMP_(void) setDepthStates(IUnknown *depthStates, UINT32 stencilReference)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11DepthStencilState> d3dDepthStencilState(depthStates);
                if (d3dDepthStencilState)
                {
                    d3dDeviceContext->OMSetDepthStencilState(d3dDepthStencilState, stencilReference);
                }
            }

            STDMETHODIMP_(void) setBlendStates(IUnknown *blendStates, const Math::Float4 &blendFactor, UINT32 mask)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11BlendState> d3dBlendState(blendStates);
                if (d3dBlendState)
                {
                    d3dDeviceContext->OMSetBlendState(d3dBlendState, blendFactor.rgba, mask);
                }
            }

            STDMETHODIMP_(void) setVertexBuffer(BufferInterface *buffer, UINT32 slot, UINT32 offset)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                if (buffer && d3dBuffer)
                {
                    UINT32 stride = buffer->getStride();
                    ID3D11Buffer *d3dBufferList[1] = { d3dBuffer };
                    d3dDeviceContext->IASetVertexBuffers(slot, 1, d3dBufferList, &stride, &offset);
                }
            }

            STDMETHODIMP_(void) setIndexBuffer(BufferInterface *buffer, UINT32 offset)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                if (buffer && d3dBuffer)
                {
                    switch (buffer->getFormat())
                    {
                    case Format::R_UINT16:
                        d3dDeviceContext->IASetIndexBuffer(d3dBuffer, DXGI_FORMAT_R16_UINT, offset);
                        break;

                    case Format::R_UINT32:
                        d3dDeviceContext->IASetIndexBuffer(d3dBuffer, DXGI_FORMAT_R32_UINT, offset);
                        break;
                    };
                }
            }

            STDMETHODIMP_(void) setPrimitiveType(PrimitiveType primitiveType)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                d3dDeviceContext->IASetPrimitiveTopology(d3dTopologList[static_cast<UINT8>(primitiveType)]);
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

        class System : public Context
                     , public Video3D::Interface
                     , public Video2D::Interface
        {
        private:
            bool isChildWindow;
            bool windowed;
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
            System(void)
                : isChildWindow(false)
                , windowed(false)
                , width(0)
                , height(0)
                , depthFormat(DXGI_FORMAT_UNKNOWN)
            {
            }

            ~System(void)
            {
                dwFactory.Release();
                d2dDeviceContext.Release();
                d2dFactory.Release();

                d3dDefaultDepthStencilView.Release();
                d3dDefaultRenderTargetView.Release();
                dxSwapChain.Release();
                d3dDevice.Release();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(Video2D::Interface)
                INTERFACE_LIST_ENTRY_COM(Video3D::Interface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, d3dDevice)
            END_INTERFACE_LIST_USER

            HRESULT createDefaultTargets(Format depthFormat)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", depthFormat, UINT32);

                this->depthFormat = DXGI_FORMAT_UNKNOWN;

                HRESULT resultValue = E_FAIL;
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

                if (SUCCEEDED(resultValue) && depthFormat != Gek::Video3D::Format::UNKNOWN)
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
                    depthDescription.Format = d3dFormatList[static_cast<UINT8>(depthFormat)];
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
                                this->depthFormat = depthDescription.Format;
                                ID3D11RenderTargetView *d3dRenderTargetViewList[1] = { d3dDefaultRenderTargetView };
                                d3dDeviceContext->OMSetRenderTargets(1, d3dRenderTargetViewList, d3dDefaultDepthStencilView);
                            }
                        }
                    }
                }

                return resultValue;
            }

            // Video3D::Interface
            STDMETHODIMP initialize(HWND window, bool windowed, UINT32 width, UINT32 height, Format depthFormat)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("0x%p", window);
                gekLogParameter("%d", windowed);
                gekLogParameter("%d", width);
                gekLogParameter("%d", height);
                gekLogTypedParameter("%d", depthFormat, UINT32);

                REQUIRE_RETURN(window, E_INVALIDARG);
                REQUIRE_RETURN(width > 0, E_INVALIDARG);
                REQUIRE_RETURN(height > 0, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                this->width = width;
                this->height = height;
                this->windowed = windowed;
                isChildWindow = (GetParent(window) != nullptr);
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
                swapChainDescription.Windowed = true;
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
                            dxDevice->GetParent(IID_PPV_ARGS(&dxAdapter));
                            if (dxAdapter)
                            {
                                CComPtr<IDXGIFactory> dxFactory;
                                dxAdapter->GetParent(IID_PPV_ARGS(&dxFactory));
                                if (dxFactory)
                                {
                                    dxFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
                                }
                            }

                            dxDevice->SetMaximumFrameLatency(1);

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
                        resultValue = createDefaultTargets(depthFormat);
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    computeSystem.reset(new ComputeSystem(d3dDeviceContext));
                    vertexSystem.reset(new VertexSystem(d3dDeviceContext));
                    geomtrySystem.reset(new GeometrySystem(d3dDeviceContext));
                    pixelSystem.reset(new PixelSystem(d3dDeviceContext));
                }

                if (SUCCEEDED(resultValue) && !windowed && !isChildWindow)
                {
                    resultValue = dxSwapChain->SetFullscreenState(true, nullptr);
                }

                return resultValue;
            }

            STDMETHODIMP resize(bool windowed, UINT32 width, UINT32 height, Format depthFormat)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("0x%p", windowed);
                gekLogParameter("%d", width);
                gekLogParameter("%d", height);
                gekLogTypedParameter("%d", depthFormat, UINT32);

                REQUIRE_RETURN(d3dDevice, E_FAIL);
                REQUIRE_RETURN(d3dDeviceContext, E_FAIL);
                REQUIRE_RETURN(dxSwapChain, E_FAIL);
                REQUIRE_RETURN(width > 0, E_INVALIDARG);
                REQUIRE_RETURN(height > 0, E_INVALIDARG);

                this->width = width;
                this->height = height;
                this->windowed = windowed;
                d2dDeviceContext->SetTarget(nullptr);
                d3dDefaultRenderTargetView.Release();
                d3dDefaultDepthStencilView.Release();

                HRESULT resultValue = S_OK;
                if (!isChildWindow)
                {
                    gekCheckResult(resultValue = dxSwapChain->SetFullscreenState(!windowed, nullptr));
                }

                if (SUCCEEDED(resultValue))
                {
                    DXGI_MODE_DESC description;
                    description.Width = width;
                    description.Height = height;
                    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    description.RefreshRate.Numerator = 60;
                    description.RefreshRate.Denominator = 1;
                    description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                    description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                    if (SUCCEEDED(gekCheckResult(resultValue = dxSwapChain->ResizeTarget(&description))))
                    {
                        if (SUCCEEDED(gekCheckResult(resultValue = dxSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))))
                        {
                            resultValue = createDefaultTargets(depthFormat);
                        }
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(UINT32) getWidth(void)
            {
                return width;
            }

            STDMETHODIMP_(UINT32) getHeight(void)
            {
                return height;
            }

            STDMETHODIMP_(bool) isWindowed(void)
            {
                return windowed;
            }

            STDMETHODIMP_(Video2D::Interface *) getVideo2D(void)
            {
                return dynamic_cast<Video2D::Interface *>(this);
            }

            STDMETHODIMP_(ContextInterface *) getDefaultContext(void)
            {
                return dynamic_cast<ContextInterface *>(this);
            }

            STDMETHODIMP createDeferredContext(ContextInterface **returnObject)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_FAIL);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
                gekCheckResult(resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext));
                if (d3dDeferredDeviceContext)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Context> deferredContext(new Context(d3dDeferredDeviceContext));
                    if (deferredContext)
                    {
                        gekCheckResult(resultValue = deferredContext->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP createEvent(IUnknown **returnObject)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                D3D11_QUERY_DESC description;
                description.Query = D3D11_QUERY_EVENT;
                description.MiscFlags = 0;

                HRESULT resultValue = E_FAIL;
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
                if (d3dQuery)
                {
                    d3dDeviceContext->End(d3dQuery);
                }
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

            STDMETHODIMP createRenderStates(IUnknown **returnObject, const Gek::Video3D::RenderStates &renderStates)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", renderStates.frontCounterClockwise, UINT32);
                gekLogParameter("%d", renderStates.depthBias);
                gekLogParameter("%f", renderStates.depthBiasClamp);
                gekLogParameter("%f", renderStates.slopeScaledDepthBias);
                gekLogTypedParameter("%d", renderStates.depthClipEnable, UINT32);
                gekLogTypedParameter("%d", renderStates.scissorEnable, UINT32);
                gekLogTypedParameter("%d", renderStates.multisampleEnable, UINT32);
                gekLogTypedParameter("%d", renderStates.antialiasedLineEnable, UINT32);
                gekLogTypedParameter("%d", renderStates.fillMode, UINT32);
                gekLogTypedParameter("%d", renderStates.cullMode, UINT32);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                D3D11_RASTERIZER_DESC rasterizerDescription;
                rasterizerDescription.FrontCounterClockwise = renderStates.frontCounterClockwise;
                rasterizerDescription.DepthBias = renderStates.depthBias;
                rasterizerDescription.DepthBiasClamp = renderStates.depthBiasClamp;
                rasterizerDescription.SlopeScaledDepthBias = renderStates.slopeScaledDepthBias;
                rasterizerDescription.DepthClipEnable = renderStates.depthClipEnable;
                rasterizerDescription.ScissorEnable = renderStates.scissorEnable;
                rasterizerDescription.MultisampleEnable = renderStates.multisampleEnable;
                rasterizerDescription.AntialiasedLineEnable = renderStates.antialiasedLineEnable;
                rasterizerDescription.FillMode = d3dFillModeList[static_cast<UINT8>(renderStates.fillMode)];
                rasterizerDescription.CullMode = d3dCullModeList[static_cast<UINT8>(renderStates.cullMode)];

                HRESULT resultValue = E_FAIL;

                CComPtr<ID3D11RasterizerState> d3dStates;
                gekCheckResult(resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates));
                if (d3dStates)
                {
                    gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createDepthStates(IUnknown **returnObject, const Gek::Video3D::DepthStates &depthStates)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", depthStates.enable, UINT32);
                gekLogTypedParameter("%d", depthStates.comparisonFunction, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilEnable, UINT32);
                gekLogParameter("%d", depthStates.stencilReadMask);
                gekLogParameter("%d", depthStates.stencilWriteMask);
                gekLogTypedParameter("%d", depthStates.stencilFrontStates.failOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilFrontStates.depthFailOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilFrontStates.passOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilFrontStates.comparisonFunction, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilBackStates.failOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilBackStates.depthFailOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilBackStates.passOperation, UINT32);
                gekLogTypedParameter("%d", depthStates.stencilBackStates.comparisonFunction, UINT32);
                gekLogParameter("%d", depthStates.writeMask);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = depthStates.enable;
                depthStencilDescription.DepthFunc = d3dComparisonFunctionList[static_cast<UINT8>(depthStates.comparisonFunction)];
                depthStencilDescription.StencilEnable = depthStates.stencilEnable;
                depthStencilDescription.StencilReadMask = depthStates.stencilReadMask;
                depthStencilDescription.StencilWriteMask = depthStates.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.failOperation)];
                depthStencilDescription.FrontFace.StencilDepthFailOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.depthFailOperation)];
                depthStencilDescription.FrontFace.StencilPassOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilFrontStates.passOperation)];
                depthStencilDescription.FrontFace.StencilFunc = d3dComparisonFunctionList[static_cast<UINT8>(depthStates.stencilFrontStates.comparisonFunction)];
                depthStencilDescription.BackFace.StencilFailOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.failOperation)];
                depthStencilDescription.BackFace.StencilDepthFailOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.depthFailOperation)];
                depthStencilDescription.BackFace.StencilPassOp = d3dStencilOperationList[static_cast<UINT8>(depthStates.stencilBackStates.passOperation)];
                depthStencilDescription.BackFace.StencilFunc = d3dComparisonFunctionList[static_cast<UINT8>(depthStates.stencilBackStates.comparisonFunction)];
                depthStencilDescription.DepthWriteMask = d3dDepthWriteMaskList[static_cast<UINT8>(depthStates.writeMask)];

                HRESULT resultValue = E_FAIL;

                CComPtr<ID3D11DepthStencilState> d3dStates;
                gekCheckResult(resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates));
                if (d3dStates)
                {
                    gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createBlendStates(IUnknown **returnObject, const Gek::Video3D::UnifiedBlendStates &blendStates)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", blendStates.alphaToCoverage, UINT32);
                gekLogTypedParameter("%d", blendStates.enable, UINT32);
                gekLogTypedParameter("%d", blendStates.colorSource, UINT32);
                gekLogTypedParameter("%d", blendStates.colorDestination, UINT32);
                gekLogTypedParameter("%d", blendStates.colorOperation, UINT32);
                gekLogTypedParameter("%d", blendStates.alphaSource, UINT32);
                gekLogTypedParameter("%d", blendStates.alphaDestination, UINT32);
                gekLogTypedParameter("%d", blendStates.alphaOperation, UINT32);
                gekLogParameter("%d", blendStates.writeMask);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
                blendDescription.IndependentBlendEnable = false;
                blendDescription.RenderTarget[0].BlendEnable = blendStates.enable;
                blendDescription.RenderTarget[0].SrcBlend = d3dBlendSourceList[static_cast<UINT8>(blendStates.colorSource)];
                blendDescription.RenderTarget[0].DestBlend = d3dBlendSourceList[static_cast<UINT8>(blendStates.colorDestination)];
                blendDescription.RenderTarget[0].BlendOp = d3dBlendOperationList[static_cast<UINT8>(blendStates.colorOperation)];
                blendDescription.RenderTarget[0].SrcBlendAlpha = d3dBlendSourceList[static_cast<UINT8>(blendStates.alphaSource)];
                blendDescription.RenderTarget[0].DestBlendAlpha = d3dBlendSourceList[static_cast<UINT8>(blendStates.alphaDestination)];
                blendDescription.RenderTarget[0].BlendOpAlpha = d3dBlendOperationList[static_cast<UINT8>(blendStates.alphaOperation)];
                blendDescription.RenderTarget[0].RenderTargetWriteMask = 0;
                if (blendStates.writeMask & Gek::Video3D::ColorMask::R)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                }

                if (blendStates.writeMask & Gek::Video3D::ColorMask::G)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                }

                if (blendStates.writeMask & Gek::Video3D::ColorMask::B)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                }

                if (blendStates.writeMask & Gek::Video3D::ColorMask::A)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                }

                HRESULT resultValue = E_FAIL;

                CComPtr<ID3D11BlendState> d3dStates;
                gekCheckResult(resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates));
                if (d3dStates)
                {
                    gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createBlendStates(IUnknown **returnObject, const Gek::Video3D::IndependentBlendStates &blendStates)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
                blendDescription.IndependentBlendEnable = true;
                for (UINT32 renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    blendDescription.RenderTarget[renderTarget].BlendEnable = blendStates.targetStates[renderTarget].enable;
                    blendDescription.RenderTarget[renderTarget].SrcBlend = d3dBlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlend = d3dBlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOp = d3dBlendOperationList[static_cast<UINT8>(blendStates.targetStates[renderTarget].colorOperation)];
                    blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = d3dBlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlendAlpha = d3dBlendSourceList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOpAlpha = d3dBlendOperationList[static_cast<UINT8>(blendStates.targetStates[renderTarget].alphaOperation)];
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                    if (blendStates.targetStates[renderTarget].writeMask & Gek::Video3D::ColorMask::R)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (blendStates.targetStates[renderTarget].writeMask & Gek::Video3D::ColorMask::G)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (blendStates.targetStates[renderTarget].writeMask & Gek::Video3D::ColorMask::B)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (blendStates.targetStates[renderTarget].writeMask & Gek::Video3D::ColorMask::A)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                HRESULT resultValue = E_FAIL;

                CComPtr<ID3D11BlendState> d3dStates;
                gekCheckResult(resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates));
                if (d3dStates)
                {
                    gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createSamplerStates(IUnknown **returnObject, const Gek::Video3D::SamplerStates &samplerStates)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", samplerStates.addressModeU, UINT32);
                gekLogTypedParameter("%d", samplerStates.addressModeV, UINT32);
                gekLogTypedParameter("%d", samplerStates.addressModeW, UINT32);
                gekLogParameter("%f", samplerStates.mipLevelBias);
                gekLogParameter("%d", samplerStates.maximumAnisotropy);
                gekLogTypedParameter("%d", samplerStates.comparisonFunction, UINT32);
                gekLogParameter("%f", samplerStates.borderColor.r);
                gekLogParameter("%f", samplerStates.borderColor.g);
                gekLogParameter("%f", samplerStates.borderColor.b);
                gekLogParameter("%f", samplerStates.borderColor.a);
                gekLogParameter("%f", samplerStates.minimumMipLevel);
                gekLogParameter("%f", samplerStates.maximumMipLevel);
                gekLogTypedParameter("%d", samplerStates.filterMode, UINT32);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                std::size_t hash = std::hash<UINT8>()(static_cast<UINT8>(samplerStates.addressModeU)) ^
                    std::hash<UINT8>()(static_cast<UINT8>(samplerStates.addressModeV)) ^
                    std::hash<UINT8>()(static_cast<UINT8>(samplerStates.addressModeW)) ^
                    std::hash<float>()(samplerStates.mipLevelBias) ^
                    std::hash<UINT32>()(samplerStates.maximumAnisotropy) ^
                    std::hash<UINT8>()(static_cast<UINT8>(samplerStates.comparisonFunction)) ^
                    std::hash<float>()(samplerStates.borderColor.r) ^
                    std::hash<float>()(samplerStates.borderColor.g) ^
                    std::hash<float>()(samplerStates.borderColor.b) ^
                    std::hash<float>()(samplerStates.borderColor.a) ^
                    std::hash<float>()(samplerStates.minimumMipLevel) ^
                    std::hash<float>()(samplerStates.maximumMipLevel) ^
                    std::hash<UINT8>()(static_cast<UINT8>(samplerStates.filterMode));

                D3D11_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = d3dAddressModeList[static_cast<UINT8>(samplerStates.addressModeU)];
                samplerDescription.AddressV = d3dAddressModeList[static_cast<UINT8>(samplerStates.addressModeV)];
                samplerDescription.AddressW = d3dAddressModeList[static_cast<UINT8>(samplerStates.addressModeW)];
                samplerDescription.MipLODBias = samplerStates.mipLevelBias;
                samplerDescription.MaxAnisotropy = samplerStates.maximumAnisotropy;
                samplerDescription.ComparisonFunc = d3dComparisonFunctionList[static_cast<UINT8>(samplerStates.comparisonFunction)];
                samplerDescription.BorderColor[0] = samplerStates.borderColor.r;
                samplerDescription.BorderColor[1] = samplerStates.borderColor.g;
                samplerDescription.BorderColor[2] = samplerStates.borderColor.b;
                samplerDescription.BorderColor[3] = samplerStates.borderColor.a;
                samplerDescription.MinLOD = samplerStates.minimumMipLevel;
                samplerDescription.MaxLOD = samplerStates.maximumMipLevel;
                samplerDescription.Filter = d3dFilterList[static_cast<UINT8>(samplerStates.filterMode)];

                HRESULT resultValue = E_FAIL;

                CComPtr<ID3D11SamplerState> d3dStates;
                gekCheckResult(resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates));
                if (d3dStates)
                {
                    gekCheckResult(resultValue = d3dStates->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createRenderTarget(TextureInterface **returnObject, UINT32 width, UINT32 height, Format format)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%d", width);
                gekLogParameter("%d", height);
                gekLogTypedParameter("%d", format, UINT32);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
                REQUIRE_RETURN(width > 0, E_INVALIDARG);
                REQUIRE_RETURN(height > 0, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                D3D11_TEXTURE2D_DESC textureDescription;
                textureDescription.Format = DXGI_FORMAT_UNKNOWN;
                textureDescription.Width = width;
                textureDescription.Height = height;
                textureDescription.MipLevels = 1;
                textureDescription.ArraySize = 1;
                textureDescription.SampleDesc.Count = 1;
                textureDescription.SampleDesc.Quality = 0;
                textureDescription.Usage = D3D11_USAGE_DEFAULT;
                textureDescription.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                textureDescription.CPUAccessFlags = 0;
                textureDescription.MiscFlags = 0;
                textureDescription.Format = d3dFormatList[static_cast<UINT8>(format)];

                CComPtr<ID3D11Texture2D> texture2D;
                gekCheckResult(resultValue = d3dDevice->CreateTexture2D(&textureDescription, nullptr, &texture2D));
                if (texture2D)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                    renderViewDescription.Format = textureDescription.Format;
                    renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    renderViewDescription.Texture2D.MipSlice = 0;

                    CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                    gekCheckResult(resultValue = d3dDevice->CreateRenderTargetView(texture2D, &renderViewDescription, &d3dRenderTargetView));
                    if (d3dRenderTargetView)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDescription;
                        shaderViewDescription.Format = textureDescription.Format;
                        shaderViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        shaderViewDescription.Texture2D.MostDetailedMip = 0;
                        shaderViewDescription.Texture2D.MipLevels = 1;

                        CComPtr<ID3D11ShaderResourceView> shaderView;
                        gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(texture2D, &shaderViewDescription, &shaderView));
                        if (shaderView)
                        {
                            resultValue = E_OUTOFMEMORY;
                            CComPtr<RenderTarget> renderTarget(new RenderTarget(width, height, shaderView, d3dRenderTargetView));
                            if (renderTarget)
                            {
                                gekCheckResult(resultValue = renderTarget->QueryInterface(IID_PPV_ARGS(returnObject)));
                            }
                        }
                    }
                }

                return resultValue;
            }

            STDMETHODIMP createDepthTarget(IUnknown **returnObject, UINT32 width, UINT32 height, Format format)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%d", width);
                gekLogParameter("%d", height);
                gekLogTypedParameter("%d", format, UINT32);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
                REQUIRE_RETURN(width > 0, E_INVALIDARG);
                REQUIRE_RETURN(height > 0, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

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
                depthDescription.Format = d3dFormatList[static_cast<UINT8>(format)];

                CComPtr<ID3D11Texture2D> texture2D;
                gekCheckResult(resultValue = d3dDevice->CreateTexture2D(&depthDescription, nullptr, &texture2D));
                if (texture2D)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = depthDescription.Format;
                    depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    CComPtr<ID3D11DepthStencilView> depthStencilView;
                    gekCheckResult(resultValue = d3dDevice->CreateDepthStencilView(texture2D, &depthStencilDescription, &depthStencilView));
                    if (depthStencilView)
                    {
                        gekCheckResult(resultValue = depthStencilView->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP createBuffer(BufferInterface **returnObject, UINT32 stride, UINT32 count, UINT32 flags, LPCVOID data)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%d", stride);
                gekLogParameter("%d", count);
                gekLogParameter("%d", flags);
                gekLogParameter("0x%p", data);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
                REQUIRE_RETURN(stride > 0, E_INVALIDARG);
                REQUIRE_RETURN(count > 0, E_INVALIDARG);

                D3D11_BUFFER_DESC bufferDescription;
                bufferDescription.ByteWidth = (stride * count);
                if (flags & Gek::Video3D::BufferFlags::STATIC)
                {
                    if (data == nullptr)
                    {
                        return E_INVALIDARG;
                    }

                    bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
                }
                else if (flags & Gek::Video3D::BufferFlags::DYNAMIC)
                {
                    bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
                }
                else
                {
                    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                }

                if (flags & Gek::Video3D::BufferFlags::VERTEX_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                }
                else if (flags & Gek::Video3D::BufferFlags::INDEX_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                }
                else if (flags & Gek::Video3D::BufferFlags::CONSTANT_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                }
                else
                {
                    bufferDescription.BindFlags = 0;
                }

                if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                if (flags & Gek::Video3D::BufferFlags::DYNAMIC)
                {
                    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                }
                else
                {
                    bufferDescription.CPUAccessFlags = 0;
                }

                if (flags & Gek::Video3D::BufferFlags::STRUCTURED_BUFFER)
                {
                    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                    bufferDescription.StructureByteStride = stride;
                }
                else
                {
                    bufferDescription.MiscFlags = 0;
                    bufferDescription.StructureByteStride = 0;
                }

                HRESULT resultValue = E_FAIL;
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
                    if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                        viewDescription.Format = DXGI_FORMAT_UNKNOWN;
                        viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = count;

                        gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView));
                    }

                    CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                    if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                        viewDescription.Format = DXGI_FORMAT_UNKNOWN;
                        viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = count;
                        viewDescription.Buffer.Flags = 0;

                        gekCheckResult(resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView));
                    }

                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Buffer> buffer(new Buffer(stride, count, d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView));
                    if (buffer)
                    {
                        gekCheckResult(resultValue = buffer->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP createBuffer(BufferInterface **returnObject, Format format, UINT32 count, UINT32 flags, LPCVOID data)
            {
                gekLogScope(__FUNCTION__);
                gekLogTypedParameter("%d", format, UINT32);
                gekLogParameter("%d", count);
                gekLogParameter("%d", flags);
                gekLogParameter("0x%p", data);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
                REQUIRE_RETURN(count > 0, E_INVALIDARG);

                UINT32 stride = d3dFormatStrideList[static_cast<UINT8>(format)];

                D3D11_BUFFER_DESC bufferDescription;
                bufferDescription.ByteWidth = (stride * count);
                if (flags & Gek::Video3D::BufferFlags::STATIC)
                {
                    if (data == nullptr)
                    {
                        return E_INVALIDARG;
                    }

                    bufferDescription.Usage = D3D11_USAGE_IMMUTABLE;
                }
                else
                {
                    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                }

                if (flags & Gek::Video3D::BufferFlags::VERTEX_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                }
                else if (flags & Gek::Video3D::BufferFlags::INDEX_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                }
                else if (flags & Gek::Video3D::BufferFlags::CONSTANT_BUFFER)
                {
                    bufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                }
                else
                {
                    bufferDescription.BindFlags = 0;
                }

                if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                bufferDescription.CPUAccessFlags = 0;
                if (flags & Gek::Video3D::BufferFlags::STRUCTURED_BUFFER)
                {
                    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                    bufferDescription.StructureByteStride = stride;
                }
                else
                {
                    bufferDescription.MiscFlags = 0;
                    bufferDescription.StructureByteStride = 0;
                }

                HRESULT resultValue = E_FAIL;
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
                    if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                        viewDescription.Format = d3dFormatList[static_cast<UINT8>(format)];
                        viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = count;

                        gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView));
                    }

                    CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                    if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                        viewDescription.Format = d3dFormatList[static_cast<UINT8>(format)];
                        viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = count;
                        viewDescription.Buffer.Flags = 0;

                        gekCheckResult(resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView));
                    }

                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Buffer> buffer(new Buffer(format, count, d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView));
                    if (buffer)
                    {
                        gekCheckResult(resultValue = buffer->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) updateBuffer(BufferInterface *buffer, LPCVOID data)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                REQUIRE_VOID_RETURN(data);

                CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                if (d3dBuffer)
                {
                    d3dDeviceContext->UpdateSubresource(d3dBuffer, 0, nullptr, data, 0, 0);
                }
            }

            STDMETHODIMP mapBuffer(BufferInterface *buffer, LPVOID *data)
            {
                REQUIRE_RETURN(d3dDeviceContext, E_FAIL);
                REQUIRE_RETURN(data, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                if (d3dBuffer)
                {
                    D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                    mappedSubResource.pData = nullptr;
                    mappedSubResource.RowPitch = 0;
                    mappedSubResource.DepthPitch = 0;
                    resultValue = d3dDeviceContext->Map(d3dBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource);
                    if (SUCCEEDED(resultValue))
                    {
                        (*data) = mappedSubResource.pData;
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) unmapBuffer(BufferInterface *buffer)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                CComQIPtr<ID3D11Buffer> d3dBuffer(buffer);
                if (d3dBuffer)
                {
                    d3dDeviceContext->Unmap(d3dBuffer, 0);
                }
            }

            STDMETHODIMP compileComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> d3dShaderMacroList;
                if (defineList != nullptr)
                {
                    for (auto &define : (*defineList))
                    {
                        D3D10_SHADER_MACRO d3dShaderMacro = { define.first.GetString(), define.second.GetString() };
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

            STDMETHODIMP compileVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Gek::Video3D::InputElement> &elementLayout, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> d3dShaderMacroList;
                if (defineList != nullptr)
                {
                    for (auto &kPair : (*defineList))
                    {
                        D3D10_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
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
                        ElementType lastElementType = Gek::Video3D::ElementType::VERTEX;
                        std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementList(elementLayout.size());
                        for (UINT32 inputElement = 0; inputElement < elementLayout.size(); ++inputElement)
                        {
                            if (lastElementType != elementLayout[inputElement].slotClass)
                            {
                                inputElementList[inputElement].AlignedByteOffset = 0;
                            }
                            else
                            {
                                inputElementList[inputElement].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                            }

                            lastElementType = elementLayout[inputElement].slotClass;
                            inputElementList[inputElement].SemanticName = elementLayout[inputElement].semanticName;
                            inputElementList[inputElement].SemanticIndex = elementLayout[inputElement].semanticIndex;
                            inputElementList[inputElement].InputSlot = elementLayout[inputElement].slotIndex;
                            switch (elementLayout[inputElement].slotClass)
                            {
                            case Gek::Video3D::ElementType::INSTANCE:
                                inputElementList[inputElement].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                                inputElementList[inputElement].InstanceDataStepRate = 1;
                                break;

                            case Gek::Video3D::ElementType::VERTEX:
                            default:
                                inputElementList[inputElement].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                                inputElementList[inputElement].InstanceDataStepRate = 0;
                            };

                            inputElementList[inputElement].Format = d3dFormatList[static_cast<UINT8>(elementLayout[inputElement].format)];
                            if (inputElementList[inputElement].Format == DXGI_FORMAT_UNKNOWN)
                            {
                                inputElementList.clear();
                                break;
                            }
                        }

                        if (!inputElementList.empty())
                        {
                            CComPtr<ID3D11InputLayout> d3dInputLayout;
                            gekCheckResult(resultValue = d3dDevice->CreateInputLayout(inputElementList.data(), inputElementList.size(), d3dShaderBlob->GetBufferPointer(), d3dShaderBlob->GetBufferSize(), &d3dInputLayout));
                            if (d3dInputLayout)
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
                }
                else if (d3dCompilerErrors)
                {
                    gekLogMessage(L"[error] %S", (LPCSTR)d3dCompilerErrors->GetBufferPointer());
                }

                return resultValue;
            }

            STDMETHODIMP compileGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList, ID3DInclude *includes)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> d3dShaderMacroList;
                if (defineList != nullptr)
                {
                    for (auto &kPair : (*defineList))
                    {
                        D3D10_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
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
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> d3dShaderMacroList;
                if (defineList != nullptr)
                {
                    for (auto &kPair : (*defineList))
                    {
                        D3D10_SHADER_MACRO d3dShaderMacro = { kPair.first.GetString(), kPair.second.GetString() };
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

            STDMETHODIMP compileComputeProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%S", entryFunction);
                return compileComputeProgram(returnObject, nullptr, programScript, entryFunction, defineList, nullptr);
            }

            STDMETHODIMP compileVertexProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, const std::vector<Gek::Video3D::InputElement> &elementLayout, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%S", entryFunction);
                return compileVertexProgram(returnObject, nullptr, programScript, entryFunction, elementLayout, defineList, nullptr);
            }

            STDMETHODIMP compileGeometryProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%S", entryFunction);
                return compileGeometryProgram(returnObject, nullptr, programScript, entryFunction, defineList, nullptr);
            }

            STDMETHODIMP compilePixelProgram(IUnknown **returnObject, LPCSTR programScript, LPCSTR entryFunction, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%S", entryFunction);
                return compilePixelProgram(returnObject, nullptr, programScript, entryFunction, defineList, nullptr);
            }

            STDMETHODIMP loadComputeProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%S", entryFunction);

                HRESULT resultValue = E_FAIL;

                CStringA progamScript;
                gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
                if (SUCCEEDED(resultValue))
                {
                    CComPtr<Include> spInclude(new Include(fileName, onInclude));
                    resultValue = compileComputeProgram(returnObject, fileName, progamScript, entryFunction, defineList, spInclude);
                }

                return resultValue;
            }

            STDMETHODIMP loadVertexProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, const std::vector<Gek::Video3D::InputElement> &elementLayout, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%S", entryFunction);

                HRESULT resultValue = E_FAIL;

                CStringA progamScript;
                gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
                if (SUCCEEDED(resultValue))
                {
                    CComPtr<Include> spInclude(new Include(fileName, onInclude));
                    resultValue = compileVertexProgram(returnObject, fileName, progamScript, entryFunction, elementLayout, defineList, spInclude);
                }

                return resultValue;
            }

            STDMETHODIMP loadGeometryProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%S", entryFunction);

                HRESULT resultValue = E_FAIL;

                CStringA progamScript;
                gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
                if (SUCCEEDED(resultValue))
                {
                    CComPtr<Include> spInclude(new Include(fileName, onInclude));
                    resultValue = compileGeometryProgram(returnObject, fileName, progamScript, entryFunction, defineList, spInclude);
                }

                return resultValue;
            }

            STDMETHODIMP loadPixelProgram(IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude, std::unordered_map<CStringA, CStringA> *defineList)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%S", entryFunction);

                HRESULT resultValue = E_FAIL;

                CStringA progamScript;
                gekCheckResult(resultValue = Gek::FileSystem::load(fileName, progamScript));
                if (SUCCEEDED(resultValue))
                {
                    CComPtr<Include> spInclude(new Include(fileName, onInclude));
                    resultValue = compilePixelProgram(returnObject, fileName, progamScript, entryFunction, defineList, spInclude);
                }

                return resultValue;
            }

            STDMETHODIMP createTexture(TextureInterface **returnObject, UINT32 width, UINT32 height, UINT32 depth, UINT8 format, UINT32 flags)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%d", width);
                gekLogParameter("%d", height);
                gekLogParameter("%d", depth);
                gekLogTypedParameter("%d", format, UINT32);
                gekLogParameter("%d", flags);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                UINT32 bindFlags = 0;
                if (flags & Gek::Video3D::TextureFlags::RESOURCE)
                {
                    bindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (flags & Gek::Video3D::TextureFlags::UNORDERED_ACCESS)
                {
                    bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                CComQIPtr<ID3D11Resource> d3dResource;
                if (depth == 1)
                {
                    D3D11_TEXTURE2D_DESC textureDescription;
                    textureDescription.Width = width;
                    textureDescription.Height = height;
                    textureDescription.MipLevels = 1;
                    textureDescription.Format = d3dFormatList[format];
                    textureDescription.ArraySize = 1;
                    textureDescription.SampleDesc.Count = 1;
                    textureDescription.SampleDesc.Quality = 0;
                    textureDescription.Usage = D3D11_USAGE_DEFAULT;
                    textureDescription.BindFlags = bindFlags;
                    textureDescription.CPUAccessFlags = 0;
                    textureDescription.MiscFlags = 0;

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
                    textureDescription.MipLevels = 1;
                    textureDescription.Format = d3dFormatList[format];
                    textureDescription.Usage = D3D11_USAGE_DEFAULT;
                    textureDescription.BindFlags = bindFlags;
                    textureDescription.CPUAccessFlags = 0;
                    textureDescription.MiscFlags = 0;

                    CComPtr<ID3D11Texture3D> texture2D;
                    gekCheckResult(resultValue = d3dDevice->CreateTexture3D(&textureDescription, nullptr, &texture2D));
                    if (texture2D)
                    {
                        d3dResource = texture2D;
                    }
                }

                if (d3dResource)
                {
                    CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                    if (flags & Gek::Video3D::TextureFlags::RESOURCE)
                    {
                        gekCheckResult(resultValue = d3dDevice->CreateShaderResourceView(d3dResource, nullptr, &d3dShaderResourceView));
                    }

                    CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                    if (flags & Gek::Video3D::TextureFlags::UNORDERED_ACCESS)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                        viewDescription.Format = d3dFormatList[format];
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

                    CComPtr<Texture> texture(new Texture(width, height, depth, d3dShaderResourceView, d3dUnorderedAccessView));
                    if (texture)
                    {
                        gekCheckResult(resultValue = texture->QueryInterface(IID_PPV_ARGS(returnObject)));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP loadTexture(TextureInterface **returnObject, LPCWSTR fileName, UINT32 flags)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);
                gekLogParameter("%d", flags);

                REQUIRE_RETURN(d3dDevice, E_INVALIDARG);
                REQUIRE_RETURN(fileName, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;

                std::vector<UINT8> fileData;
                if (SUCCEEDED(Gek::FileSystem::load(fileName, fileData)))
                {
                    DirectX::ScratchImage scratchImage;
                    DirectX::TexMetadata textureMetaData;
                    if (FAILED(DirectX::LoadFromDDSMemory(fileData.data(), fileData.size(), 0, &textureMetaData, scratchImage)))
                    {
                        if (FAILED(DirectX::LoadFromTGAMemory(fileData.data(), fileData.size(), &textureMetaData, scratchImage)))
                        {
                            static const DWORD formatList[] =
                            {
                                DirectX::WIC_CODEC_PNG,              // Portable Network Graphics (.png)
                                DirectX::WIC_CODEC_BMP,              // Windows Bitmap (.bmp)
                                DirectX::WIC_CODEC_JPEG,             // Joint Photographic Experts Group (.jpg, .jpeg)
                            };

                            for (UINT32 format = 0; format < _ARRAYSIZE(formatList); format++)
                            {
                                if (SUCCEEDED(DirectX::LoadFromWICMemory(fileData.data(), fileData.size(), formatList[format], &textureMetaData, scratchImage)))
                                {
                                    break;
                                }
                            }
                        }
                    }

                    CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                    gekCheckResult(resultValue = DirectX::CreateShaderResourceView(d3dDevice, scratchImage.GetImages(), scratchImage.GetImageCount(), textureMetaData, &d3dShaderResourceView));
                    if (d3dShaderResourceView)
                    {
                        CComPtr<Texture> texture2D(new Texture(textureMetaData.width, textureMetaData.height, textureMetaData.depth, d3dShaderResourceView, nullptr));
                        if (texture2D)
                        {
                            gekCheckResult(resultValue = texture2D->QueryInterface(IID_PPV_ARGS(returnObject)));
                        }
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) updateTexture(TextureInterface *texture, LPCVOID data, UINT32 pitch, Shape::Rectangle<UINT32> *destinationRectangle)
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

            STDMETHODIMP_(void) clearDefaultRenderTarget(const Math::Float4 &colorClear)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                REQUIRE_VOID_RETURN(d3dDefaultRenderTargetView);
                d3dDeviceContext->ClearRenderTargetView(d3dDefaultRenderTargetView, colorClear.rgba);
            }

            STDMETHODIMP_(void) clearDefaultDepthStencilTarget(UINT32 flags, float depthClear, UINT32 stencilClear)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext);
                REQUIRE_VOID_RETURN(d3dDefaultDepthStencilView);
                d3dDeviceContext->ClearDepthStencilView(d3dDefaultDepthStencilView,
                    ((flags & Gek::Video3D::ClearMask::DEPTH ? D3D11_CLEAR_DEPTH : 0) |
                    (flags & Gek::Video3D::ClearMask::STENCIL ? D3D11_CLEAR_STENCIL : 0)),
                     depthClear, stencilClear);
            }

            STDMETHODIMP_(void) setDefaultTargets(ContextInterface *context, IUnknown *depthBuffer)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext || context);
                REQUIRE_VOID_RETURN(d3dDefaultRenderTargetView);
                REQUIRE_VOID_RETURN(d3dDefaultDepthStencilView);

                D3D11_VIEWPORT viewPortList[1];
                viewPortList[0].TopLeftX = 0.0f;
                viewPortList[0].TopLeftY = 0.0f;
                viewPortList[0].Width = float(width);
                viewPortList[0].Height = float(height);
                viewPortList[0].MinDepth = 0.0f;
                viewPortList[0].MaxDepth = 1.0f;

                ID3D11RenderTargetView *d3dRenderTargetViewList[1] = { d3dDefaultRenderTargetView };
                CComQIPtr<ID3D11DepthStencilView> d3dDepthStencilView(depthBuffer ? depthBuffer : (IUnknown *)d3dDefaultDepthStencilView);
                CComQIPtr<ID3D11DeviceContext> d3dDeferredContext(context);
                if (d3dDeferredContext)
                {
                    d3dDeferredContext->OMSetRenderTargets(1, d3dRenderTargetViewList, d3dDepthStencilView);
                    d3dDeferredContext->RSSetViewports(1, viewPortList);
                }
                else
                {
                    d3dDeviceContext->OMSetRenderTargets(1, d3dRenderTargetViewList, d3dDepthStencilView);
                    d3dDeviceContext->RSSetViewports(1, viewPortList);
                }
            }

            STDMETHODIMP_(void) executeCommandList(IUnknown *commandList)
            {
                REQUIRE_VOID_RETURN(d3dDeviceContext && commandList);

                CComQIPtr<ID3D11CommandList> d3dCommandList(commandList);
                if (d3dCommandList)
                {
                    d3dDeviceContext->ExecuteCommandList(d3dCommandList, FALSE);
                }
            }

            STDMETHODIMP_(void) present(bool waitForVerticalSync)
            {
                REQUIRE_VOID_RETURN(dxSwapChain);

                dxSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
            }

            // Video2D::Interface
            STDMETHODIMP createBrush(IUnknown **returnObject, const Math::Float4 &color)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%f", color.r);
                gekLogParameter("%f", color.g);
                gekLogParameter("%f", color.b);
                gekLogParameter("%f", color.a);

                REQUIRE_RETURN(d2dDeviceContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComPtr<ID2D1SolidColorBrush> d2dSolidBrush;
                gekCheckResult(resultValue = d2dDeviceContext->CreateSolidColorBrush(*(D2D1_COLOR_F *)&color, &d2dSolidBrush));
                if (d2dSolidBrush)
                {
                    gekCheckResult(resultValue = d2dSolidBrush->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP createBrush(IUnknown **returnObject, const std::vector<GradientPoint> &stopPoints, const Shape::Rectangle<float> &extents)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(d2dDeviceContext, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
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

            STDMETHODIMP createFont(IUnknown **returnObject, LPCWSTR face, UINT32 weight, FontStyle style, float size)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", face);
                gekLogParameter("%d", weight);
                gekLogTypedParameter("%d", style, UINT32);
                gekLogParameter("%f", size);

                REQUIRE_RETURN(d2dDeviceContext, E_INVALIDARG);
                REQUIRE_RETURN(face, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComPtr<IDWriteTextFormat> dwTextFormat;
                gekCheckResult(resultValue = dwFactory->CreateTextFormat(face, nullptr, DWRITE_FONT_WEIGHT(weight), d2dFontStyleList[static_cast<UINT8>(style)], DWRITE_FONT_STRETCH_NORMAL, size, L"", &dwTextFormat));
                if (dwTextFormat)
                {
                    gekCheckResult(resultValue = dwTextFormat->QueryInterface(IID_PPV_ARGS(returnObject)));
                }

                return resultValue;
            }

            STDMETHODIMP loadBitmap(IUnknown **returnObject, LPCWSTR fileName)
            {
                gekLogScope(__FUNCTION__);
                gekLogParameter("%s", fileName);

                HRESULT resultValue = E_FAIL;
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

            STDMETHODIMP createGeometry(GeometryInterface **returnObject)
            {
                gekLogScope(__FUNCTION__);

                REQUIRE_RETURN(dwFactory, E_FAIL);
                REQUIRE_RETURN(returnObject, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComPtr<ID2D1PathGeometry> d2dPathGeometry;
                gekCheckResult(resultValue = d2dFactory->CreatePathGeometry(&d2dPathGeometry));
                if (d2dPathGeometry)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Geometry> geometry(new Geometry(d2dPathGeometry));
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

            STDMETHODIMP_(void) drawText(const Shape::Rectangle<float> &extents, IUnknown *font, IUnknown *brush, LPCWSTR format, ...)
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

            STDMETHODIMP_(void) drawRectangle(const Shape::Rectangle<float> &extents, IUnknown *brush, bool fillShape)
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

            STDMETHODIMP_(void) drawRectangle(const Shape::Rectangle<float> &extents, const Math::Float2 &cornerRadius, IUnknown *brush, bool fillShape)
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

            STDMETHODIMP_(void) drawBitmap(IUnknown *bitmap, const Shape::Rectangle<float> &destinationExtents, InterpolationMode interpolationMode, float opacity)
            {
                REQUIRE_VOID_RETURN(d2dDeviceContext);
                CComQIPtr<ID2D1Bitmap1> d2dBitmap(bitmap);
                if (d2dBitmap)
                {
                    d2dDeviceContext->DrawBitmap(d2dBitmap, (const D2D1_RECT_F *)&destinationExtents, opacity, d2dInterpolationMode[static_cast<UINT8>(interpolationMode)]);
                }
            }

            STDMETHODIMP_(void) drawBitmap(IUnknown *bitmap, const Shape::Rectangle<float> &destinationExtents, const Shape::Rectangle<float> &sourceExtents, InterpolationMode interpolationMode, float opacity)
            {
                REQUIRE_VOID_RETURN(d2dDeviceContext);
                CComQIPtr<ID2D1Bitmap1> d2dBitmap(bitmap);
                if (d2dBitmap)
                {
                    d2dDeviceContext->DrawBitmap(d2dBitmap, (const D2D1_RECT_F *)&destinationExtents, opacity, d2dInterpolationMode[static_cast<UINT8>(interpolationMode)], (const D2D1_RECT_F *)&sourceExtents);
                }
            }

            STDMETHODIMP_(void) drawGeometry(GeometryInterface *pGeometry, IUnknown *brush, bool fillShape)
            {
                REQUIRE_VOID_RETURN(d2dDeviceContext);
                REQUIRE_VOID_RETURN(pGeometry);

                CComQIPtr<ID2D1PathGeometry> d2dPathGeometry(pGeometry);
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

        REGISTER_CLASS(System);
    }; // namespace Video
}; // namespace Gek
