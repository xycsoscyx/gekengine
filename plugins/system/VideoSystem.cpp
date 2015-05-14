#pragma warning(disable : 4005)

#include "GEK\Context\Common.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\System\Video2DInterface.h"
#include "GEK\System\Video3DInterface.h"
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
        static const DXGI_FORMAT gs_aFormats[] =
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

        static const UINT32 gs_aFormatStrides[] = 
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

        static const D3D11_DEPTH_WRITE_MASK gs_aDepthWriteMasks[] =
        {
            D3D11_DEPTH_WRITE_MASK_ZERO,
            D3D11_DEPTH_WRITE_MASK_ALL,
        };

        static const D3D11_TEXTURE_ADDRESS_MODE gs_aAddressModes[] = 
        {
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
            D3D11_TEXTURE_ADDRESS_BORDER,
        };

        static const D3D11_COMPARISON_FUNC gs_aComparisonFunctions[] = 
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

        static const D3D11_STENCIL_OP gs_aStencilOperations[] = 
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

        static const D3D11_BLEND gs_aBlendSources[] =
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

        static const D3D11_BLEND_OP gs_aBlendOperations[] = 
        {
            D3D11_BLEND_OP_ADD,
            D3D11_BLEND_OP_SUBTRACT,
            D3D11_BLEND_OP_REV_SUBTRACT,
            D3D11_BLEND_OP_MIN,
            D3D11_BLEND_OP_MAX,
        };

        static const D3D11_FILL_MODE gs_aFillModes[] =
        {
            D3D11_FILL_WIREFRAME,
            D3D11_FILL_SOLID,
        };

        static const D3D11_CULL_MODE gs_aCullModes[] =
        {
            D3D11_CULL_NONE,
            D3D11_CULL_FRONT,
            D3D11_CULL_BACK,
        };

        static const D3D11_FILTER gs_aFilters[] =
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

        static const D3D11_PRIMITIVE_TOPOLOGY gs_aTopology[] = 
        {
            D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

        static const DWRITE_FONT_STYLE gs_aFontStyles[] =
        {
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STYLE_ITALIC,
        };

        DECLARE_INTERFACE(ResourceManagerInterface)
        {
            STDMETHOD_(IUnknown *, GetResource)                 (THIS_ Handle resourceHandle) PURE;
        };

        class VertexProgram : public ContextUser
        {
        private:
            CComPtr<ID3D11VertexShader> vertexShader;
            CComPtr<ID3D11InputLayout> inputLayout;

        public:
            VertexProgram(ID3D11VertexShader *vertexShader, ID3D11InputLayout *inputLayout)
                : vertexShader(vertexShader)
                , inputLayout(inputLayout)
            {
            }

            BEGIN_INTERFACE_LIST(VertexProgram)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11VertexShader, vertexShader)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11InputLayout, inputLayout)
            END_INTERFACE_LIST_UNKNOWN
        };

        DECLARE_INTERFACE_IID_(BufferInterface, IUnknown, "5FFBBB66-158D-469D-8A95-6AD938B5C37D")
        {
            STDMETHOD_(UINT32, getStride)               (THIS) PURE;
        };

        class Buffer : public ContextUser
                     , public BufferInterface
        {
        private:
            CComPtr<ID3D11Buffer> buffer;
            CComPtr<ID3D11ShaderResourceView> shaderResourceView;
            CComPtr<ID3D11UnorderedAccessView> unorderedAccessView;
            UINT32 stride;

        public:
            Buffer(ID3D11Buffer *buffer, UINT32 stride = 0, ID3D11ShaderResourceView *shaderResourceView = nullptr, ID3D11UnorderedAccessView *unorderedAccessView = nullptr)
                : buffer(buffer)
                , shaderResourceView(shaderResourceView)
                , unorderedAccessView(unorderedAccessView)
                , stride(stride)
            {
            }

            BEGIN_INTERFACE_LIST(Buffer)
                INTERFACE_LIST_ENTRY_COM(BufferInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Buffer, buffer)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, shaderResourceView)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, unorderedAccessView)
            END_INTERFACE_LIST_UNKNOWN

            // BufferInterface
            STDMETHODIMP_(UINT32) getStride(void)
            {
                return stride;
            }
        };

        class Texture : public ContextUser
        {
        protected:
            CComPtr<ID3D11ShaderResourceView> shaderResourceView;
            CComPtr<ID3D11UnorderedAccessView> unorderedAccessView;

        public:
            Texture(ID3D11ShaderResourceView *shaderResourceView, ID3D11UnorderedAccessView *unorderedAccessView)
                : shaderResourceView(shaderResourceView)
                , unorderedAccessView(unorderedAccessView)
            {
            }

            BEGIN_INTERFACE_LIST(Texture)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11ShaderResourceView, shaderResourceView)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11UnorderedAccessView, unorderedAccessView)
            END_INTERFACE_LIST_UNKNOWN
        };

        class RenderTarget : public Texture
        {
        private:
            CComPtr<ID3D11RenderTargetView> renderTargetView;

        public:
            RenderTarget(ID3D11ShaderResourceView *shaderResourceView, ID3D11UnorderedAccessView *unorderedAccessView, ID3D11RenderTargetView *renderTargetView)
                : Texture(shaderResourceView, unorderedAccessView)
                , renderTargetView(renderTargetView)
            {
            }

            BEGIN_INTERFACE_LIST(RenderTarget)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11RenderTargetView, renderTargetView)
            END_INTERFACE_LIST_BASE(Texture)
        };

        class Geometry : public ContextUser
                       , public GeometryInterface
        {
        private:
            CComPtr<ID2D1PathGeometry> pathGeometry;
            CComPtr<ID2D1GeometrySink> geometrySink;
            bool figureHasBegun;

        public:
            Geometry(ID2D1PathGeometry *pGeometry)
                : pathGeometry(pGeometry)
                , figureHasBegun(false)
            {
            }

            BEGIN_INTERFACE_LIST(Geometry)
                INTERFACE_LIST_ENTRY_COM(GeometryInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID2D1PathGeometry, pathGeometry)
            END_INTERFACE_LIST_UNKNOWN

            // GeometryInterface
            STDMETHODIMP openShape(void)
            {
                REQUIRE_RETURN(pathGeometry, E_FAIL);

                geometrySink = nullptr;
                return pathGeometry->Open(&geometrySink);
            }

            STDMETHODIMP closeShape(void)
            {
                REQUIRE_RETURN(pathGeometry && geometrySink, E_FAIL);

                HRESULT resultValue = geometrySink->Close();
                if (SUCCEEDED(resultValue))
                {
                    geometrySink = nullptr;
                }

                return resultValue;
            }

            STDMETHODIMP_(void) beginModifications(const Math::Float2 &point, bool fillShape)
            {
                REQUIRE_VOID_RETURN(pathGeometry && geometrySink);

                if (!figureHasBegun)
                {
                    figureHasBegun = true;
                    geometrySink->BeginFigure(*(D2D1_POINT_2F *)&point, fillShape ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);
                }
            }

            STDMETHODIMP_(void) endModifications(bool openEnded)
            {
                REQUIRE_VOID_RETURN(pathGeometry && geometrySink);

                if (figureHasBegun)
                {
                    figureHasBegun = false;
                    geometrySink->EndFigure(openEnded ? D2D1_FIGURE_END_OPEN : D2D1_FIGURE_END_CLOSED);
                }
            }

            STDMETHODIMP_(void) addLine(const Math::Float2 &point)
            {
                REQUIRE_VOID_RETURN(geometrySink);

                if (figureHasBegun)
                {
                    geometrySink->AddLine(*(D2D1_POINT_2F *)&point);
                }
            }

            STDMETHODIMP_(void) addBezier(const Math::Float2 &point1, const Math::Float2 &point2, const Math::Float2 &point3)
            {
                REQUIRE_VOID_RETURN(geometrySink);

                if (figureHasBegun)
                {
                    geometrySink->AddBezier({ *(D2D1_POINT_2F *)&point1, *(D2D1_POINT_2F *)&point2, *(D2D1_POINT_2F *)&point3 });
                }
            }

            STDMETHODIMP createWidened(float width, float tolerance, GeometryInterface **instance)
            {
                REQUIRE_RETURN(pathGeometry && instance, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComPtr<ID2D1Factory> factory;
                pathGeometry->GetFactory(&factory);
                if (factory)
                {
                    CComPtr<ID2D1PathGeometry> widenedGeometry;
                    resultValue = factory->CreatePathGeometry(&widenedGeometry);
                    if (widenedGeometry)
                    {
                        CComPtr<ID2D1GeometrySink> widenedSink;
                        resultValue = widenedGeometry->Open(&widenedSink);
                        if (widenedSink)
                        {
                            resultValue = pathGeometry->Widen(width, nullptr, nullptr, tolerance, widenedSink);
                            if (SUCCEEDED(resultValue))
                            {
                                widenedSink->Close();
                                resultValue = E_OUTOFMEMORY;
                                CComPtr<Geometry> geometry(new Geometry(widenedGeometry));
                                if (geometry)
                                {
                                    resultValue = geometry->QueryInterface(IID_PPV_ARGS(instance));
                                }
                            }
                        }
                    }
                }

                return resultValue;
            }
        };

        class Include : public ContextUser
                      , public ID3DInclude
        {
        private:
            CPathW shaderFilePath;
            std::vector<UINT8> buffer;

        public:
            Include(const CStringW &shaderFileName)
                : shaderFilePath(shaderFileName)
            {
                shaderFilePath.RemoveFileSpec();
            }

            BEGIN_INTERFACE_LIST(Include)
                INTERFACE_LIST_ENTRY_COM(IUnknown)
            END_INTERFACE_LIST_UNKNOWN

            // ID3DInclude
            STDMETHODIMP Open(D3D_INCLUDE_TYPE includeType, LPCSTR fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize)
            {
                REQUIRE_RETURN(fileName && data && dataSize, E_INVALIDARG);

                buffer.clear();
                HRESULT resultValue = Gek::FileSystem::load(CA2W(fileName), buffer);
                if (FAILED(resultValue))
                {
                    CPathW shaderPath;
                    shaderPath.Combine(shaderFilePath, CA2W(fileName));
                    resultValue = Gek::FileSystem::load(shaderPath, buffer);
                }

                if (SUCCEEDED(resultValue))
                {
                    (*data) = buffer.data();
                    (*dataSize) = buffer.size();
                }

                return resultValue;
            }

            STDMETHODIMP Close(LPCVOID data)
            {
                return (data == buffer.data() ? S_OK : E_FAIL);
            }
        };

        class Context : public ContextUser
                      , public Video3D::ContextInterface
        {
            friend class System;

        protected:
            class System
            {
            protected:
                ResourceManagerInterface *resourceHandler;
                ID3D11DeviceContext *deviceContext;

            public:
                System(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                    : deviceContext(deviceContext)
                    , resourceHandler(resourceHandler)
                {
                }
            };

            class ComputeSystem : public System
                                , public SubSystemInterface
            {
            public:
                ComputeSystem(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                    : System(deviceContext, resourceHandler)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(Handle resourceHandle)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11ComputeShader> shaderResource(resourceHandler->GetResource(resourceHandle));
                    if (shaderResource)
                    {
                        deviceContext->CSSetShader(shaderResource, nullptr, 0);
                    }
                    else
                    {
                        deviceContext->CSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11Buffer> bufferResource(resourceHandler->GetResource(resourceHandle));
                    if (bufferResource)
                    {
                        ID3D11Buffer *bufferList[1] = { bufferResource };
                        deviceContext->CSSetConstantBuffers(nStage, 1, bufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11SamplerState> statesResource(resourceHandler->GetResource(resourceHandle));
                    if (statesResource)
                    {
                        ID3D11SamplerState *statesList[1] = { statesResource };
                        deviceContext->CSSetSamplers(nStage, 1, statesList);
                    }
                }

                STDMETHODIMP_(void) setResource(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    ID3D11ShaderResourceView *viewsList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> viewResource(resourceHandler->GetResource(resourceHandle));
                    if (viewResource)
                    {
                        viewsList[0] = viewResource;
                    }

                    deviceContext->CSSetShaderResources(nStage, 1, viewsList);
                }

                STDMETHODIMP_(void) setUnorderedAccess(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    ID3D11UnorderedAccessView *viewsList[1] = { nullptr };
                    CComQIPtr<ID3D11UnorderedAccessView> viewResource(resourceHandler->GetResource(resourceHandle));
                    if (viewResource)
                    {
                        viewsList[0] = viewResource;
                    }

                    deviceContext->CSSetUnorderedAccessViews(nStage, 1, viewsList, nullptr);
                }
            };

            class VertexSystem : public System
                               , public SubSystemInterface
            {
            public:
                VertexSystem(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                    : System(deviceContext, resourceHandler)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(Handle resourceHandle)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    IUnknown *resource = resourceHandler->GetResource(resourceHandle);
                    CComQIPtr<ID3D11VertexShader> shaderResource(resource);
                    CComQIPtr<ID3D11InputLayout> inputLayoutResource(resource);
                    if (shaderResource && inputLayoutResource)
                    {
                        deviceContext->VSSetShader(shaderResource, nullptr, 0);
                        deviceContext->IASetInputLayout(inputLayoutResource);
                    }
                    else
                    {
                        deviceContext->VSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11Buffer> bufferResource(resourceHandler->GetResource(resourceHandle));
                    if (bufferResource)
                    {
                        ID3D11Buffer *bufferList[1] = { bufferResource };
                        deviceContext->VSSetConstantBuffers(nStage, 1, bufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11SamplerState> statesResource(resourceHandler->GetResource(resourceHandle));
                    if (statesResource)
                    {
                        ID3D11SamplerState *statesList[1] = { statesResource };
                        deviceContext->VSSetSamplers(nStage, 1, statesList);
                    }
                }

                STDMETHODIMP_(void) setResource(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    ID3D11ShaderResourceView *viewsList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> viewResource(resourceHandler->GetResource(resourceHandle));
                    if (viewResource)
                    {
                        viewsList[0] = viewResource;
                    }

                    deviceContext->VSSetShaderResources(nStage, 1, viewsList);
                }
            };

            class GeometrySystem : public System
                                 , public SubSystemInterface
            {
            public:
                GeometrySystem(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                    : System(deviceContext, resourceHandler)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(Handle resourceHandle)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11GeometryShader> shaderResource(resourceHandler->GetResource(resourceHandle));
                    if (shaderResource)
                    {
                        deviceContext->GSSetShader(shaderResource, nullptr, 0);
                    }
                    else
                    {
                        deviceContext->GSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11Buffer> bufferResource(resourceHandler->GetResource(resourceHandle));
                    if (bufferResource)
                    {
                        ID3D11Buffer *bufferList[1] = { bufferResource };
                        deviceContext->GSSetConstantBuffers(nStage, 1, bufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11SamplerState> statesResource(resourceHandler->GetResource(resourceHandle));
                    if (statesResource)
                    {
                        ID3D11SamplerState *statesList[1] = { statesResource };
                        deviceContext->GSSetSamplers(nStage, 1, statesList);
                    }
                }

                STDMETHODIMP_(void) setResource(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    ID3D11ShaderResourceView *viewsList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> viewResource(resourceHandler->GetResource(resourceHandle));
                    if (viewResource)
                    {
                        viewsList[0] = viewResource;
                    }

                    deviceContext->GSSetShaderResources(nStage, 1, viewsList);
                }
            };

            class PixelSystem : public System
                              , public SubSystemInterface
            {
            public:
                PixelSystem(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                    : System(deviceContext, resourceHandler)
                {
                }

                // SubSystemInterface
                STDMETHODIMP_(void) setProgram(Handle resourceHandle)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11PixelShader> shaderResource(resourceHandler->GetResource(resourceHandle));
                    if (shaderResource)
                    {
                        deviceContext->PSSetShader(shaderResource, nullptr, 0);
                    }
                    else
                    {
                        deviceContext->PSSetShader(nullptr, nullptr, 0);
                    }
                }

                STDMETHODIMP_(void) setConstantBuffer(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11Buffer> bufferResource(resourceHandler->GetResource(resourceHandle));
                    if (bufferResource)
                    {
                        ID3D11Buffer *bufferList[1] = { bufferResource };
                        deviceContext->PSSetConstantBuffers(nStage, 1, bufferList);
                    }
                }

                STDMETHODIMP_(void) setSamplerStates(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    CComQIPtr<ID3D11SamplerState> statesResource(resourceHandler->GetResource(resourceHandle));
                    if (statesResource)
                    {
                        ID3D11SamplerState *statesList[1] = { statesResource };
                        deviceContext->PSSetSamplers(nStage, 1, statesList);
                    }
                }

                STDMETHODIMP_(void) setResource(Handle resourceHandle, UINT32 nStage)
                {
                    REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                    ID3D11ShaderResourceView *viewsList[1] = { nullptr };
                    CComQIPtr<ID3D11ShaderResourceView> viewResource(resourceHandler->GetResource(resourceHandle));
                    if (viewResource)
                    {
                        viewsList[0] = viewResource;
                    }

                    deviceContext->PSSetShaderResources(nStage, 1, viewsList);
                }
            };

        protected:
            ResourceManagerInterface *resourceHandler;
            CComPtr<ID3D11DeviceContext> deviceContext;
            std::unique_ptr<SubSystemInterface> computeSystem;
            std::unique_ptr<SubSystemInterface> vertexSystem;
            std::unique_ptr<SubSystemInterface> geomtrySystem;
            std::unique_ptr<SubSystemInterface> pixelSystem;

        public:
            Context(ResourceManagerInterface *resourceHandler)
                : resourceHandler(resourceHandler)
            {
            }

            Context(ID3D11DeviceContext *deviceContext, ResourceManagerInterface *resourceHandler)
                : resourceHandler(resourceHandler)
                , deviceContext(deviceContext)
                , computeSystem(new ComputeSystem(deviceContext, resourceHandler))
                , vertexSystem(new VertexSystem(deviceContext, resourceHandler))
                , geomtrySystem(new GeometrySystem(deviceContext, resourceHandler))
                , pixelSystem(new PixelSystem(deviceContext, resourceHandler))
            {
            }

            BEGIN_INTERFACE_LIST(Context)
                INTERFACE_LIST_ENTRY_COM(ContextInterface)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11DeviceContext, deviceContext)
            END_INTERFACE_LIST_UNKNOWN

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
                static ID3D11ShaderResourceView *const nullResourceViews[] =
                {
                    nullptr, nullptr, nullptr, nullptr, nullptr,
                    nullptr, nullptr, nullptr, nullptr, nullptr,
                };

                static ID3D11RenderTargetView  *const nullRenderTargets[] =
                {
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                };

                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->CSSetShaderResources(0, 10, nullResourceViews);
                deviceContext->VSSetShaderResources(0, 10, nullResourceViews);
                deviceContext->GSSetShaderResources(0, 10, nullResourceViews);
                deviceContext->PSSetShaderResources(0, 10, nullResourceViews);
                deviceContext->OMSetRenderTargets(6, nullRenderTargets, nullptr);
            }

            STDMETHODIMP_(void) setViewports(const std::vector<Gek::Video3D::ViewPort> &viewPorts)
            {
                REQUIRE_VOID_RETURN(deviceContext && viewPorts.size() > 0);
                deviceContext->RSSetViewports(viewPorts.size(), (D3D11_VIEWPORT *)viewPorts.data());
            }

            STDMETHODIMP_(void) setScissorRect(const std::vector<Rectangle<UINT32>> &rectangles)
            {
                REQUIRE_VOID_RETURN(deviceContext && rectangles.size() > 0);
                deviceContext->RSSetScissorRects(rectangles.size(), (D3D11_RECT *)rectangles.data());
            }

            STDMETHODIMP_(void) clearRenderTarget(Handle targetHandle, const Math::Float4 &color)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                CComQIPtr<ID3D11RenderTargetView> targetResource(resourceHandler->GetResource(targetHandle));
                if (targetResource)
                {
                    deviceContext->ClearRenderTargetView(targetResource, color.rgba);
                }
            }

            STDMETHODIMP_(void) clearDepthStencilTarget(Handle depthHandle, UINT32 flags, float depth, UINT32 stencil)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                CComQIPtr<ID3D11DepthStencilView> depthResource(resourceHandler->GetResource(depthHandle));
                if (depthResource)
                {
                    deviceContext->ClearDepthStencilView(depthResource,
                        ((flags & Gek::Video3D::ClearMask::DEPTH ? D3D11_CLEAR_DEPTH : 0) |
                         (flags & Gek::Video3D::ClearMask::STENCIL ? D3D11_CLEAR_STENCIL : 0)),
                          depth, stencil);
                }
            }

            STDMETHODIMP_(void) setRenderTargets(const std::vector<Gek::Handle> &targetHandleList, Handle depthHandle)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                std::vector<ID3D11RenderTargetView *> targetResourceList;
                for (auto &targetHandle : targetHandleList)
                {
                    CComQIPtr<ID3D11RenderTargetView> targetResource(resourceHandler->GetResource(targetHandle));
                    targetResourceList.push_back(targetResource);
                }

                CComQIPtr<ID3D11DepthStencilView> depthResource(resourceHandler->GetResource(depthHandle));
                if (depthResource)
                {
                    deviceContext->OMSetRenderTargets(targetResourceList.size(), targetResourceList.data(), depthResource);
                }
                else
                {
                    deviceContext->OMSetRenderTargets(targetResourceList.size(), targetResourceList.data(), nullptr);
                }
            }

            STDMETHODIMP_(void) setRenderStates(Handle resourceHandle)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                CComQIPtr<ID3D11RasterizerState> statesResource(resourceHandler->GetResource(resourceHandle));
                if (statesResource)
                {
                    deviceContext->RSSetState(statesResource);
                }
            }

            STDMETHODIMP_(void) setDepthStates(Handle resourceHandle, UINT32 nStencilReference)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                CComQIPtr<ID3D11DepthStencilState> statesResource(resourceHandler->GetResource(resourceHandle));
                if (statesResource)
                {
                    deviceContext->OMSetDepthStencilState(statesResource, nStencilReference);
                }
            }

            STDMETHODIMP_(void) setBlendStates(Handle resourceHandle, const Math::Float4 &nBlendFactor, UINT32 nMask)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                CComQIPtr<ID3D11BlendState> statesResource(resourceHandler->GetResource(resourceHandle));
                if (statesResource)
                {
                    deviceContext->OMSetBlendState(statesResource, nBlendFactor.rgba, nMask);
                }
            }

            STDMETHODIMP_(void) setVertexBuffer(Handle resourceHandle, UINT32 nSlot, UINT32 nOffset)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                IUnknown *resource = resourceHandler->GetResource(resourceHandle);
                CComQIPtr<BufferInterface> bufferData(resource);
                CComQIPtr<ID3D11Buffer> buffer(resource);
                if (bufferData && buffer)
                {
                    UINT32 stride = bufferData->getStride();
                    ID3D11Buffer *pBuffer = buffer;
                    deviceContext->IASetVertexBuffers(nSlot, 1, &pBuffer, &stride, &nOffset);
                }
            }

            STDMETHODIMP_(void) setIndexBuffer(Handle resourceHandle, UINT32 nOffset)
            {
                REQUIRE_VOID_RETURN(deviceContext && resourceHandler);
                IUnknown *resource = resourceHandler->GetResource(resourceHandle);
                CComQIPtr<BufferInterface> bufferData(resource);
                CComQIPtr<ID3D11Buffer> buffer(resource);
                if (bufferData && buffer)
                {
                    switch (bufferData->getStride())
                    {
                    case 2:
                        deviceContext->IASetIndexBuffer(buffer, DXGI_FORMAT_R16_UINT, nOffset);
                        break;

                    case 4:
                        deviceContext->IASetIndexBuffer(buffer, DXGI_FORMAT_R32_UINT, nOffset);
                        break;
                    };
                }
            }

            STDMETHODIMP_(void) setPrimitiveType(UINT8 primitiveType)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->IASetPrimitiveTopology(gs_aTopology[primitiveType]);
            }

            STDMETHODIMP_(void) drawPrimitive(UINT32 vertexCount, UINT32 firstVertex)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->Draw(vertexCount, firstVertex);
            }

            STDMETHODIMP_(void) drawInstancedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 vertexCount, UINT32 firstVertex)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
            }

            STDMETHODIMP_(void) drawIndexedPrimitive(UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
            }

            STDMETHODIMP_(void) drawInstancedIndexedPrimitive(UINT32 instanceCount, UINT32 firstInstance, UINT32 indexCount, UINT32 firstIndex, UINT32 firstVertex)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
            }

            STDMETHODIMP_(void) dispatch(UINT32 threadGroupCountX, UINT32 threadGroupCountY, UINT32 threadGroupCountZ)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                deviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
            }

            STDMETHODIMP_(void) finishCommandList(IUnknown **instance)
            {
                REQUIRE_VOID_RETURN(deviceContext && instance);

                CComPtr<ID3D11CommandList> commandList;
                deviceContext->FinishCommandList(FALSE, &commandList);
                if (commandList)
                {
                    commandList->QueryInterface(IID_PPV_ARGS(instance));
                }
            }
        };

        class System : public Context
                     , public ResourceManagerInterface
                     , public Video3D::SystemInterface
                     , public Video2D::SystemInterface
        {
            struct Resource
            {
                CComPtr<IUnknown> data;
                std::function<void(CComPtr<IUnknown> &)> onFree;
                std::function<void(CComPtr<IUnknown> &)> onRestore;

                Resource(IUnknown *data, std::function<void(CComPtr<IUnknown> &)> onFree = nullptr, std::function<void(CComPtr<IUnknown> &)> onRestore = nullptr)
                    : data(data)
                    , onFree(onFree)
                    , onRestore(onRestore)
                {
                }
            };

        private:
            bool windowed;
            UINT32 width;
            UINT32 height;
            DXGI_FORMAT depthFormat;

            CComPtr<ID3D11Device> device;
            CComPtr<IDXGISwapChain> swapChain;
            CComPtr<ID3D11RenderTargetView> defaultRenderTargetView;
            CComPtr<ID3D11DepthStencilView> defaultDepthStencilView;

            CComPtr<ID2D1Factory1> factory2D;
            CComPtr<ID2D1DeviceContext> deviceContext2D;
            CComPtr<IDWriteFactory> factoryWrite;

            Gek::Handle nextResourceHandle;
            concurrency::concurrent_unordered_map<Gek::Handle, Resource> resourceList;

        public:
            System(void)
                : Context(this)
                , nextResourceHandle(Gek::InvalidHandle)
                , windowed(false)
                , width(0)
                , height(0)
                , depthFormat(DXGI_FORMAT_UNKNOWN)
            {
            }

            ~System(void)
            {
                resourceList.clear();
            }

            BEGIN_INTERFACE_LIST(System)
                INTERFACE_LIST_ENTRY_COM(IGEKObservable)
                INTERFACE_LIST_ENTRY_COM(IGEK3DVideoSystem)
                INTERFACE_LIST_ENTRY_COM(IGEK2DVideoSystem)
                INTERFACE_LIST_ENTRY_MEMBER(IID_ID3D11Device, device)
            END_INTERFACE_LIST_BASE(Context)

            HRESULT getDefaultTargets(UINT8 depthFormat)
            {
                this->depthFormat = DXGI_FORMAT_UNKNOWN;

                CComPtr<IDXGISurface> swapBuffer;
                HRESULT resultValue = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapBuffer));
                if (swapBuffer)
                {
                    FLOAT desktopWidth = 0.0f;
                    FLOAT desptopHeight = 0.0f;
                    factory2D->GetDesktopDpi(&desktopWidth, &desptopHeight);

                    D2D1_BITMAP_PROPERTIES1 desktopProperties;
                    desktopProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
                    desktopProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
                    desktopProperties.dpiX = desktopWidth;
                    desktopProperties.dpiY = desptopHeight;
                    desktopProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
                    desktopProperties.colorContext = nullptr;

                    CComPtr<ID2D1Bitmap1> bitmap;
                    resultValue = deviceContext2D->CreateBitmapFromDxgiSurface(swapBuffer, &desktopProperties, &bitmap);
                    if (bitmap)
                    {
                        deviceContext2D->SetTarget(bitmap);
                    }
                }

                if (SUCCEEDED(resultValue) && depthFormat != Gek::Video3D::Format::UNKNOWN)
                {
                    CComPtr<ID3D11Texture2D> swapTexture;
                    resultValue = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapTexture));
                    if (swapTexture)
                    {
                        resultValue = device->CreateRenderTargetView(swapTexture, nullptr, &defaultRenderTargetView);
                        if (defaultRenderTargetView)
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
                            depthDescription.Format = gs_aFormats[depthFormat];
                            if (depthDescription.Format != DXGI_FORMAT_UNKNOWN)
                            {
                                CComPtr<ID3D11Texture2D> depthTexture;
                                resultValue = device->CreateTexture2D(&depthDescription, nullptr, &depthTexture);
                              f  if (depthTexture)
                                {
                                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                                    depthStencilDescription.Format = depthDescription.Format;
                                    depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                                    depthStencilDescription.Flags = 0;
                                    depthStencilDescription.Texture2D.MipSlice = 0;
                                    resultValue = device->CreateDepthStencilView(depthTexture, &depthStencilDescription, &defaultDepthStencilView);
                                    if (defaultDepthStencilView)
                                    {
                                        this->depthFormat = depthDescription.Format;
                                        ID3D11RenderTargetView *renderTargetView = defaultRenderTargetView;
                                        deviceContext->OMSetRenderTargets(1, &renderTargetView, defaultDepthStencilView);
                                    }
                                }
                            }
                        }
                    }
                }

                return resultValue;
            }

            // ResourceManagerInterface
            STDMETHODIMP_(IUnknown *) getResource(Handle resourceHandle)
            {
                auto resourceIterator = resourceList.find(resourceHandle);
                if (resourceIterator == resourceList.end())
                {
                    return nullptr;
                }
                else
                {
                    return (*resourceIterator).second.data;
                }
            }

            // Video3D::SystemInterface
                STDMETHODIMP initialize(HWND window, bool windowed, UINT32 width, UINT32 height, UINT8 depthFormat)
            {
                this->width = width;
                this->height = height;
                this->windowed = windowed;
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
                HRESULT resultValue = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, _ARRAYSIZE(featureLevelList), D3D11_SDK_VERSION, &swapChainDescription, &swapChain, &device, &featureLevel, &deviceContext);
                if (device && deviceContext && swapChain)
                {
                    resultValue = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, IID_PPV_ARGS(&factory2D));
                    if (factory2D)
                    {
                        resultValue = E_FAIL;
                        CComQIPtr<IDXGIDevice1> deviceGI(device);
                        if (deviceGI)
                        {
                            CComPtr<IDXGIAdapter> adapterGI;
                            deviceGI->GetParent(IID_PPV_ARGS(&adapterGI));
                            if (adapterGI)
                            {
                                CComPtr<IDXGIFactory> factoryGI;
                                adapterGI->GetParent(IID_PPV_ARGS(&factoryGI));
                                if (factoryGI)
                                {
                                    factoryGI->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
                                }
                            }

                            deviceGI->SetMaximumFrameLatency(1);

                            CComPtr<ID2D1Device> device2D;
                            resultValue = factory2D->CreateDevice(deviceGI, &device2D);
                            if (device2D)
                            {
                                resultValue = device2D->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &deviceContext2D);
                            }
                        }
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&factoryWrite));
                    }

                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = getDefaultTargets(depthFormat);
                    }
                }

                if (SUCCEEDED(resultValue))
                {
                    computeSystem.reset(new ComputeSystem(deviceContext, this));
                    vertexSystem.reset(new VertexSystem(deviceContext, this));
                    geomtrySystem.reset(new GeometrySystem(deviceContext, this));
                    pixelSystem.reset(new PixelSystem(deviceContext, this));
                }

                if (SUCCEEDED(resultValue) && !windowed)
                {
                    resultValue = swapChain->SetFullscreenState(true, nullptr);
                }

                return resultValue;
            }

            STDMETHODIMP resize(bool windowed, UINT32 width, UINT32 height, UINT8 depthFormat)
            {
                REQUIRE_RETURN(device, E_FAIL);
                REQUIRE_RETURN(deviceContext, E_FAIL);
                REQUIRE_RETURN(deviceContext2D, E_FAIL);

                this->width = width;
                this->height = height;
                this->windowed = windowed;
                deviceContext2D->SetTarget(nullptr);
                defaultRenderTargetView.Release();
                defaultDepthStencilView.Release();
                for (auto resource : resourceList)
                {
                    if (resource.second.onFree)
                    {
                        resource.second.onFree(resource.second.data);
                    }
                }

                HRESULT resultValue = swapChain->SetFullscreenState(!windowed, nullptr);
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
                    resultValue = swapChain->ResizeTarget(&description);
                    if (SUCCEEDED(resultValue))
                    {
                        resultValue = swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                        if (SUCCEEDED(resultValue))
                        {
                            resultValue = getDefaultTargets(depthFormat);
                            if (SUCCEEDED(resultValue))
                            {
                                for (auto resource : resourceList)
                                {
                                    if (resource.second.onRestore)
                                    {
                                        resource.second.onRestore(resource.second.data);
                                    }
                                }
                            }
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

            STDMETHODIMP createDeferredContext(ContextInterface **instance)
            {
                REQUIRE_RETURN(device, E_FAIL);
                REQUIRE_RETURN(instance, E_INVALIDARG);

                CComPtr<ID3D11DeviceContext> deferredContext;
                HRESULT resultValue = device->CreateDeferredContext(0, &deferredContext);
                if (deferredContext)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Context> context(new Context(deferredContext, this));
                    if (context)
                    {
                        resultValue = context->QueryInterface(IID_PPV_ARGS(instance));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) freeResource(Handle resourceHandle)
            {
                auto resourceIterator = resourceList.find(resourceHandle);
                if (resourceIterator != resourceList.end())
                {
                    resourceList.unsafe_erase(resourceIterator);
                }
            }

            STDMETHODIMP_(Gek::Handle) createEvent(void)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                D3D11_QUERY_DESC description;
                description.Query = D3D11_QUERY_EVENT;
                description.MiscFlags = 0;

                CComPtr<ID3D11Query> queryEvent;
                device->CreateQuery(&description, &queryEvent);
                if (queryEvent)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(queryEvent)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(void) setEvent(Handle resourceHandle)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                CComQIPtr<ID3D11Query> queryEvent(GetResource(resourceHandle));
                if (queryEvent)
                {
                    deviceContext->End(queryEvent);
                }
            }

            STDMETHODIMP_(bool) isEventSet(Handle resourceHandle)
            {
                REQUIRE_RETURN(deviceContext, false);

                bool isEventSet = false;
                CComQIPtr<ID3D11Query> queryEvent(GetResource(resourceHandle));
                if (queryEvent)
                {
                    UINT32 nIsSet = 0;
                    if (SUCCEEDED(deviceContext->GetData(queryEvent, (LPVOID)&nIsSet, sizeof(UINT32), TRUE)))
                    {
                        isEventSet = (nIsSet == 1);
                    }
                }

                return isEventSet;
            }

            STDMETHODIMP_(Gek::Handle) createRenderStates(const Gek::Video3D::RenderStates &renderStates)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                D3D11_RASTERIZER_DESC rasterizerDescription;
                rasterizerDescription.FrontCounterClockwise = renderStates.frontCounterClockwise;
                rasterizerDescription.DepthBias = renderStates.depthBias;
                rasterizerDescription.DepthBiasClamp = renderStates.depthBiasClamp;
                rasterizerDescription.SlopeScaledDepthBias = renderStates.slopeScaledDepthBias;
                rasterizerDescription.DepthClipEnable = renderStates.depthClipEnable;
                rasterizerDescription.ScissorEnable = renderStates.scissorEnable;
                rasterizerDescription.MultisampleEnable = renderStates.multisampleEnable;
                rasterizerDescription.AntialiasedLineEnable = renderStates.antialiasedLineEnable;
                rasterizerDescription.FillMode = gs_aFillModes[renderStates.fillMode];
                rasterizerDescription.CullMode = gs_aCullModes[renderStates.cullMode];

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID3D11RasterizerState> states;
                device->CreateRasterizerState(&rasterizerDescription, &states);
                if (states)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(states)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createDepthStates(const Gek::Video3D::DepthStates &depthStates)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = depthStates.depthEnable;
                depthStencilDescription.DepthFunc = gs_aComparisonFunctions[depthStates.depthComparison];
                depthStencilDescription.StencilEnable = depthStates.stencilEnable;
                depthStencilDescription.StencilReadMask = depthStates.stencilReadMask;
                depthStencilDescription.StencilWriteMask = depthStates.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = gs_aStencilOperations[depthStates.stencilFrontStates.stencilFailOperation];
                depthStencilDescription.FrontFace.StencilDepthFailOp = gs_aStencilOperations[depthStates.stencilFrontStates.stencilDepthFailOperation];
                depthStencilDescription.FrontFace.StencilPassOp = gs_aStencilOperations[depthStates.stencilFrontStates.stencilPassOperation];
                depthStencilDescription.FrontFace.StencilFunc = gs_aComparisonFunctions[depthStates.stencilFrontStates.stencilComparison];
                depthStencilDescription.BackFace.StencilFailOp = gs_aStencilOperations[depthStates.stencilBackStates.stencilFailOperation];
                depthStencilDescription.BackFace.StencilDepthFailOp = gs_aStencilOperations[depthStates.stencilBackStates.stencilDepthFailOperation];
                depthStencilDescription.BackFace.StencilPassOp = gs_aStencilOperations[depthStates.stencilBackStates.stencilPassOperation];
                depthStencilDescription.BackFace.StencilFunc = gs_aComparisonFunctions[depthStates.stencilBackStates.stencilComparison];
                depthStencilDescription.DepthWriteMask = gs_aDepthWriteMasks[depthStates.depthWriteMask];

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID3D11DepthStencilState> states;
                device->CreateDepthStencilState(&depthStencilDescription, &states);
                if (states)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(states)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createBlendStates(const Gek::Video3D::UnifiedBlendStates &blendStates)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
                blendDescription.IndependentBlendEnable = false;
                blendDescription.RenderTarget[0].BlendEnable = blendStates.enable;
                blendDescription.RenderTarget[0].SrcBlend = gs_aBlendSources[blendStates.colorSource];
                blendDescription.RenderTarget[0].DestBlend = gs_aBlendSources[blendStates.colorDestination];
                blendDescription.RenderTarget[0].BlendOp = gs_aBlendOperations[blendStates.colorOperation];
                blendDescription.RenderTarget[0].SrcBlendAlpha = gs_aBlendSources[blendStates.alphaSource];
                blendDescription.RenderTarget[0].DestBlendAlpha = gs_aBlendSources[blendStates.alphaDestination];
                blendDescription.RenderTarget[0].BlendOpAlpha = gs_aBlendOperations[blendStates.alphaOperation];
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

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID3D11BlendState> states;
                device->CreateBlendState(&blendDescription, &states);
                if (states)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(states)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createBlendStates(const Gek::Video3D::IndependentBlendStates &blendStates)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStates.alphaToCoverage;
                blendDescription.IndependentBlendEnable = true;
                for (UINT32 nTarget = 0; nTarget < 8; ++nTarget)
                {
                    blendDescription.RenderTarget[nTarget].BlendEnable = blendStates.targetStates[nTarget].enable;
                    blendDescription.RenderTarget[nTarget].SrcBlend = gs_aBlendSources[blendStates.targetStates[nTarget].colorSource];
                    blendDescription.RenderTarget[nTarget].DestBlend = gs_aBlendSources[blendStates.targetStates[nTarget].colorDestination];
                    blendDescription.RenderTarget[nTarget].BlendOp = gs_aBlendOperations[blendStates.targetStates[nTarget].colorOperation];
                    blendDescription.RenderTarget[nTarget].SrcBlendAlpha = gs_aBlendSources[blendStates.targetStates[nTarget].alphaSource];
                    blendDescription.RenderTarget[nTarget].DestBlendAlpha = gs_aBlendSources[blendStates.targetStates[nTarget].alphaDestination];
                    blendDescription.RenderTarget[nTarget].BlendOpAlpha = gs_aBlendOperations[blendStates.targetStates[nTarget].alphaOperation];
                    blendDescription.RenderTarget[nTarget].RenderTargetWriteMask = 0;
                    if (blendStates.targetStates[nTarget].writeMask & Gek::Video3D::ColorMask::R)
                    {
                        blendDescription.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (blendStates.targetStates[nTarget].writeMask & Gek::Video3D::ColorMask::G)
                    {
                        blendDescription.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (blendStates.targetStates[nTarget].writeMask & Gek::Video3D::ColorMask::B)
                    {
                        blendDescription.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (blendStates.targetStates[nTarget].writeMask & Gek::Video3D::ColorMask::A)
                    {
                        blendDescription.RenderTarget[nTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID3D11BlendState> states;
                device->CreateBlendState(&blendDescription, &states);
                if (states)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(states)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createSamplerStates(const Gek::Video3D::SamplerStates &samplerStates)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                D3D11_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = gs_aAddressModes[samplerStates.addressModeU];
                samplerDescription.AddressV = gs_aAddressModes[samplerStates.addressModeV];
                samplerDescription.AddressW = gs_aAddressModes[samplerStates.addressModeW];
                samplerDescription.MipLODBias = samplerStates.mipLevelBias;
                samplerDescription.MaxAnisotropy = samplerStates.maximumAnisotropy;
                samplerDescription.ComparisonFunc = gs_aComparisonFunctions[samplerStates.comparisonFunction];
                samplerDescription.BorderColor[0] = samplerStates.borderColor.r;
                samplerDescription.BorderColor[1] = samplerStates.borderColor.g;
                samplerDescription.BorderColor[2] = samplerStates.borderColor.b;
                samplerDescription.BorderColor[3] = samplerStates.borderColor.a;
                samplerDescription.MinLOD = samplerStates.minimumMipLevel;
                samplerDescription.MaxLOD = samplerStates.maximumMipLevel;
                samplerDescription.Filter = gs_aFilters[samplerStates.filterMode];

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID3D11SamplerState> states;
                device->CreateSamplerState(&samplerDescription, &states);
                if (states)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, Resource(states)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createRenderTarget(UINT32 width, UINT32 height, UINT8 format)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);
                REQUIRE_RETURN(width > 0 && height > 0, Gek::InvalidHandle);

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
                textureDescription.Format = gs_aFormats[format];

                Gek::Handle resourceHandle = Gek::InvalidHandle;
                if (textureDescription.Format != DXGI_FORMAT_UNKNOWN)
                {
                    CComPtr<ID3D11Texture2D> texture2D;
                    device->CreateTexture2D(&textureDescription, nullptr, &texture2D);
                    if (texture2D)
                    {
                        D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                        renderViewDescription.Format = textureDescription.Format;
                        renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                        renderViewDescription.Texture2D.MipSlice = 0;

                        CComPtr<ID3D11RenderTargetView> renderTargetView;
                        device->CreateRenderTargetView(texture2D, &renderViewDescription, &renderTargetView);
                        if (renderTargetView)
                        {
                            D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDescription;
                            shaderViewDescription.Format = textureDescription.Format;
                            shaderViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                            shaderViewDescription.Texture2D.MostDetailedMip = 0;
                            shaderViewDescription.Texture2D.MipLevels = 1;

                            CComPtr<ID3D11ShaderResourceView> shaderView;
                            device->CreateShaderResourceView(texture2D, &shaderViewDescription, &shaderView);
                            if (shaderView)
                            {
                                CComPtr<RenderTarget> textureResource(new RenderTarget(shaderView, nullptr, renderTargetView));
                                if (textureResource)
                                {
                                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                                    resourceList.insert(std::make_pair(resourceHandle, Resource(textureResource, [](CComPtr<IUnknown> &textureResource) -> void
                                    {
                                        textureResource.Release();
                                    }, std::bind([&](CComPtr<IUnknown> &textureResource, UINT32 width, UINT32 height, D3D11_TEXTURE2D_DESC restoreDescription) -> void
                                    {
                                        CComPtr<ID3D11Texture2D> texture2D;
                                        device->CreateTexture2D(&restoreDescription, nullptr, &texture2D);
                                        if (texture2D)
                                        {
                                            D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                                            renderViewDescription.Format = restoreDescription.Format;
                                            renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                                            renderViewDescription.Texture2D.MipSlice = 0;

                                            CComPtr<ID3D11RenderTargetView> renderTargetView;
                                            device->CreateRenderTargetView(texture2D, &renderViewDescription, &renderTargetView);
                                            if (renderTargetView)
                                            {
                                                D3D11_SHADER_RESOURCE_VIEW_DESC shaderViewDescription;
                                                shaderViewDescription.Format = restoreDescription.Format;
                                                shaderViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                                                shaderViewDescription.Texture2D.MostDetailedMip = 0;
                                                shaderViewDescription.Texture2D.MipLevels = 1;

                                                CComPtr<ID3D11ShaderResourceView> shaderView;
                                                device->CreateShaderResourceView(texture2D, &shaderViewDescription, &shaderView);
                                                if (shaderView)
                                                {
                                                    CComPtr<RenderTarget> textureResource(new RenderTarget(shaderView, nullptr, renderTargetView));
                                                    if (textureResource)
                                                    {
                                                        textureResource = textureResource;
                                                    }
                                                }
                                            }
                                        }
                                    }, std::placeholders::_1, width, height, textureDescription))));
                                }
                            }
                        }
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createDepthTarget(UINT32 width, UINT32 height, UINT8 format)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);
                REQUIRE_RETURN(width > 0 && height > 0, Gek::InvalidHandle);

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
                depthDescription.Format = gs_aFormats[format];

                Gek::Handle resourceHandle = Gek::InvalidHandle;
                if (depthDescription.Format != DXGI_FORMAT_UNKNOWN)
                {
                    CComPtr<ID3D11Texture2D> texture2D;
                    device->CreateTexture2D(&depthDescription, nullptr, &texture2D);
                    if (texture2D)
                    {
                        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                        depthStencilDescription.Format = depthDescription.Format;
                        depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        depthStencilDescription.Flags = 0;
                        depthStencilDescription.Texture2D.MipSlice = 0;

                        CComPtr<ID3D11DepthStencilView> depthStencilView;
                        device->CreateDepthStencilView(texture2D, &depthStencilDescription, &depthStencilView);
                        if (depthStencilView)
                        {
                            resourceHandle = InterlockedIncrement(&nextResourceHandle);
                            resourceList.insert(std::make_pair(resourceHandle, RESOURCE(depthStencilView)));
                        }
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createBuffer(UINT32 stride, UINT32 count, UINT32 flags, LPCVOID data)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);
                REQUIRE_RETURN(stride > 0 && count > 0, Gek::InvalidHandle);

                D3D11_BUFFER_DESC bufferDescription;
                bufferDescription.ByteWidth = (stride * count);
                if (flags & Gek::Video3D::BufferFlags::STATIC)
                {
                    if (data == nullptr)
                    {
                        return Gek::InvalidHandle;
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

                CComPtr<ID3D11Buffer> buffer;
                if (data == nullptr)
                {
                    device->CreateBuffer(&bufferDescription, nullptr, &buffer);
                }
                else
                {
                    D3D11_SUBRESOURCE_DATA resourceData;
                    resourceData.pSysMem = data;
                    resourceData.SysMemPitch = 0;
                    resourceData.SysMemSlicePitch = 0;
                    device->CreateBuffer(&bufferDescription, &resourceData, &buffer);
                }

                Gek::Handle resourceHandle = Gek::InvalidHandle;
                if (buffer)
                {
                    CComPtr<ID3D11ShaderResourceView> spShaderView;
                    if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                        kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
                        kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                        kViewDesc.Buffer.FirstElement = 0;
                        kViewDesc.Buffer.NumElements = count;

                        device->CreateShaderResourceView(buffer, &kViewDesc, &spShaderView);
                    }

                    CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
                    if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                        kViewDesc.Format = DXGI_FORMAT_UNKNOWN;
                        kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                        kViewDesc.Buffer.FirstElement = 0;
                        kViewDesc.Buffer.NumElements = count;
                        kViewDesc.Buffer.Flags = 0;

                        device->CreateUnorderedAccessView(buffer, &kViewDesc, &spUnorderedView);
                    }

                    CComPtr<Buffer> spBuffer(new Buffer(spBuffer, stride, spShaderView, spUnorderedView));
                    if (spBuffer)
                    {
                        resourceHandle = InterlockedIncrement(&nextResourceHandle);
                        resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spBuffer->GetUnknown())));
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createBuffer(UINT8 format, UINT32 count, UINT32 flags, LPCVOID data)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);
                REQUIRE_RETURN(format != Gek::Video3D::Format::UNKNOWN && count > 0, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;
                UINT32 stride = gs_aFormatStrides[format];
                DXGI_FORMAT eNewFormat = gs_aFormats[format];
                if (eNewFormat != DXGI_FORMAT_UNKNOWN)
                {
                    D3D11_BUFFER_DESC bufferDescription;
                    bufferDescription.ByteWidth = (stride * count);
                    if (flags & Gek::Video3D::BufferFlags::STATIC)
                    {
                        if (data == nullptr)
                        {
                            return Gek::InvalidHandle;
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

                    CComPtr<ID3D11Buffer> spBuffer;
                    if (data == nullptr)
                    {
                        device->CreateBuffer(&bufferDescription, nullptr, &spBuffer);
                    }
                    else
                    {
                        D3D11_SUBRESOURCE_DATA resourceData;
                        resourceData.pSysMem = data;
                        resourceData.SysMemPitch = 0;
                        resourceData.SysMemSlicePitch = 0;
                        device->CreateBuffer(&bufferDescription, &resourceData, &spBuffer);
                    }

                    if (spBuffer)
                    {
                        CComPtr<ID3D11ShaderResourceView> spShaderView;
                        if (flags & Gek::Video3D::BufferFlags::RESOURCE)
                        {
                            D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                            kViewDesc.Format = eNewFormat;
                            kViewDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                            kViewDesc.Buffer.FirstElement = 0;
                            kViewDesc.Buffer.NumElements = count;

                            device->CreateShaderResourceView(spBuffer, &kViewDesc, &spShaderView);
                        }

                        CComPtr<ID3D11UnorderedAccessView> spUnorderedView;
                        if (flags & Gek::Video3D::BufferFlags::UNORDERED_ACCESS)
                        {
                            D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                            kViewDesc.Format = eNewFormat;
                            kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                            kViewDesc.Buffer.FirstElement = 0;
                            kViewDesc.Buffer.NumElements = count;
                            kViewDesc.Buffer.Flags = 0;

                            device->CreateUnorderedAccessView(spBuffer, &kViewDesc, &spUnorderedView);
                        }

                        CComPtr<Buffer> spBuffer(new Buffer(spBuffer, stride, spShaderView, spUnorderedView));
                        if (spBuffer)
                        {
                            resourceHandle = InterlockedIncrement(&nextResourceHandle);
                            resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spBuffer->GetUnknown())));
                        }
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(void) UpdateBuffer(Handle resourceHandle, LPCVOID data)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                REQUIRE_VOID_RETURN(data);

                CComQIPtr<ID3D11Buffer> spBuffer(GetResource(resourceHandle));
                if (spBuffer)
                {
                    deviceContext->UpdateSubresource(spBuffer, 0, nullptr, data, 0, 0);
                }
            }

            STDMETHODIMP MapBuffer(Handle resourceHandle, LPVOID *data)
            {
                REQUIRE_RETURN(deviceContext, E_FAIL);
                REQUIRE_RETURN(data, E_INVALIDARG);

                HRESULT resultValue = E_FAIL;
                CComQIPtr<ID3D11Buffer> spBuffer(GetResource(resourceHandle));
                if (spBuffer)
                {
                    D3D11_MAPPED_SUBRESOURCE resource;
                    resource.data = nullptr;
                    resource.RowPitch = 0;
                    resource.DepthPitch = 0;
                    resultValue = deviceContext->Map(spBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
                    if (SUCCEEDED(resultValue))
                    {
                        (*data) = resource.data;
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) UnMapBuffer(Handle resourceHandle)
            {
                REQUIRE_VOID_RETURN(deviceContext);
                CComQIPtr<ID3D11Buffer> spBuffer(GetResource(resourceHandle));
                if (spBuffer)
                {
                    deviceContext->Unmap(spBuffer, 0);
                }
            }

            Gek::Handle CompileComputeProgram(LPCWSTR fileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> aDefines;
                if (pDefines != nullptr)
                {
                    for (auto &kPair : (*pDefines))
                    {
                        D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
                        aDefines.push_back(kMacro);
                    }
                }

                static const D3D10_SHADER_MACRO kMacro =
                {
                    nullptr,
                    nullptr
                };

                aDefines.push_back(kMacro);

                CComPtr<ID3DBlob> spBlob;
                CComPtr<ID3DBlob> spErrors;
                D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(fileName), aDefines.data(), pIncludes, pEntry, "cs_5_0", flags, 0, &spBlob, &spErrors);
                if (spBlob)
                {
                    CComPtr<ID3D11ComputeShader> spProgram;
                    device->CreateComputeShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
                    if (spProgram)
                    {
                        resourceHandle = InterlockedIncrement(&nextResourceHandle);
                        resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spProgram)));
                    }
                }
                else if (spErrors)
                {
                    OutputDebugStringA(FormatString("Compute Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
                }

                return resourceHandle;
            }

            Gek::Handle CompileVertexProgram(LPCWSTR fileName, LPCSTR pProgram, LPCSTR pEntry, const std::vector<Gek::Video3D::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> aDefines;
                if (pDefines != nullptr)
                {
                    for (auto &kPair : (*pDefines))
                    {
                        D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
                        aDefines.push_back(kMacro);
                    }
                }

                D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
                aDefines.push_back(kMacro);

                CComPtr<ID3DBlob> spBlob;
                CComPtr<ID3DBlob> spErrors;
                D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(fileName), aDefines.data(), pIncludes, pEntry, "vs_5_0", flags, 0, &spBlob, &spErrors);
                if (spBlob)
                {
                    CComPtr<ID3D11VertexShader> spProgram;
                    device->CreateVertexShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
                    if (spProgram)
                    {
                        Gek::Video3D::INPUT::SOURCE eLastClass = Gek::Video3D::INPUT::UNKNOWN;
                        std::vector<D3D11_INPUT_ELEMENT_DESC> aLayoutDesc(aLayout.size());
                        for (UINT32 nIndex = 0; nIndex < aLayout.size(); ++nIndex)
                        {
                            if (eLastClass != aLayout[nIndex].class)
                            {
                                aLayoutDesc[nIndex].AlignedByteOffset = 0;
                            }
                            else
                            {
                                aLayoutDesc[nIndex].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                            }

                            eLastClass = aLayout[nIndex].class;
                            aLayoutDesc[nIndex].SemanticName = aLayout[nIndex].m_pName;
                            aLayoutDesc[nIndex].SemanticIndex = aLayout[nIndex].m_nIndex;
                            aLayoutDesc[nIndex].InputSlot = aLayout[nIndex].slot;
                            switch (aLayout[nIndex].class)
                            {
                            case Gek::Video3D::INPUT::INSTANCE:
                                aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                                aLayoutDesc[nIndex].InstanceDataStepRate = 1;
                                break;

                            case Gek::Video3D::INPUT::VERTEX:
                            default:
                                aLayoutDesc[nIndex].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                                aLayoutDesc[nIndex].InstanceDataStepRate = 0;
                            };

                            aLayoutDesc[nIndex].Format = gs_aFormats[aLayout[nIndex].m_eType];
                            if (aLayoutDesc[nIndex].Format == DXGI_FORMAT_UNKNOWN)
                            {
                                aLayoutDesc.clear();
                                break;
                            }
                        }

                        if (!aLayoutDesc.empty())
                        {
                            CComPtr<ID3D11InputLayout> spLayout;
                            device->CreateInputLayout(aLayoutDesc.data(), aLayoutDesc.size(), spBlob->GetBufferPointer(), spBlob->GetBufferSize(), &spLayout);
                            if (spLayout)
                            {
                                CComPtr<VertexProgram> spProgram(new VertexProgram(spProgram, spLayout));
                                if (spProgram)
                                {
                                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                                    resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spProgram)));
                                }
                            }
                        }
                    }
                }
                else if (spErrors)
                {
                    OutputDebugStringA(FormatString("Vertex Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
                }

                return resourceHandle;
            }

            Gek::Handle CompileGeometryProgram(LPCWSTR fileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> aDefines;
                if (pDefines != nullptr)
                {
                    for (auto &kPair : (*pDefines))
                    {
                        D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
                        aDefines.push_back(kMacro);
                    }
                }

                D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
                aDefines.push_back(kMacro);

                CComPtr<ID3DBlob> spBlob;
                CComPtr<ID3DBlob> spErrors;
                D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(fileName), aDefines.data(), pIncludes, pEntry, "gs_5_0", flags, 0, &spBlob, &spErrors);
                if (spBlob)
                {
                    CComPtr<ID3D11GeometryShader> spProgram;
                    device->CreateGeometryShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
                    if (spProgram)
                    {
                        resourceHandle = InterlockedIncrement(&nextResourceHandle);
                        resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spProgram)));
                    }
                }
                else if (spErrors)
                {
                    OutputDebugStringA(FormatString("Geometry Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
                }

                return resourceHandle;
            }

            Gek::Handle CompilePixelProgram(LPCWSTR fileName, LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines, ID3DInclude *pIncludes)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
#endif

                std::vector<D3D10_SHADER_MACRO> aDefines;
                if (pDefines != nullptr)
                {
                    for (auto &kPair : (*pDefines))
                    {
                        D3D10_SHADER_MACRO kMacro = { kPair.first.GetString(), kPair.second.GetString() };
                        aDefines.push_back(kMacro);
                    }
                }

                D3D10_SHADER_MACRO kMacro = { nullptr, nullptr };
                aDefines.push_back(kMacro);

                CComPtr<ID3DBlob> spBlob;
                CComPtr<ID3DBlob> spErrors;
                D3DCompile(pProgram, (strlen(pProgram) + 1), CW2A(fileName), aDefines.data(), pIncludes, pEntry, "ps_5_0", flags, 0, &spBlob, &spErrors);
                if (spBlob)
                {
                    CComPtr<ID3D11PixelShader> spProgram;
                    device->CreatePixelShader(spBlob->GetBufferPointer(), spBlob->GetBufferSize(), nullptr, &spProgram);
                    if (spProgram)
                    {
                        resourceHandle = InterlockedIncrement(&nextResourceHandle);
                        resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spProgram)));
                    }
                }
                else if (spErrors)
                {
                    OutputDebugStringA(FormatString("Pixel Error: %s", (LPCSTR)spErrors->GetBufferPointer()));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) compileComputeProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                return CompileComputeProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
            }

            STDMETHODIMP_(Gek::Handle) compileVertexProgram(LPCSTR pProgram, LPCSTR pEntry, const std::vector<Gek::Video3D::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                return CompileVertexProgram(nullptr, pProgram, pEntry, aLayout, pDefines, nullptr);
            }

            STDMETHODIMP_(Gek::Handle) compileGeometryProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                return CompileGeometryProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
            }

            STDMETHODIMP_(Gek::Handle) compilePixelProgram(LPCSTR pProgram, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                return CompilePixelProgram(nullptr, pProgram, pEntry, pDefines, nullptr);
            }

            STDMETHODIMP_(Gek::Handle) loadComputeProgram(LPCWSTR fileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CStringA strProgram;
                if (SUCCEEDED(GEKLoadFromFile(fileName, strProgram)))
                {
                    CComPtr<Include> spInclude(new Include(fileName));
                    resourceHandle = CompileComputeProgram(fileName, strProgram, pEntry, pDefines, spInclude);
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) loadVertexProgram(LPCWSTR fileName, LPCSTR pEntry, const std::vector<Gek::Video3D::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CStringA strProgram;
                if (SUCCEEDED(GEKLoadFromFile(fileName, strProgram)))
                {
                    CComPtr<Include> spInclude(new Include(fileName));
                    resourceHandle = CompileVertexProgram(fileName, strProgram, pEntry, aLayout, pDefines, spInclude);
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) loadGeometryProgram(LPCWSTR fileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CStringA strProgram;
                if (SUCCEEDED(GEKLoadFromFile(fileName, strProgram)))
                {
                    CComPtr<Include> spInclude(new Include(fileName));
                    resourceHandle = CompileGeometryProgram(fileName, strProgram, pEntry, pDefines, spInclude);
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) loadPixelProgram(LPCWSTR fileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
            {
                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CStringA strProgram;
                if (SUCCEEDED(GEKLoadFromFile(fileName, strProgram)))
                {
                    CComPtr<Include> spInclude(new Include(fileName));
                    resourceHandle = CompilePixelProgram(fileName, strProgram, pEntry, pDefines, spInclude);
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createTexture(UINT32 width, UINT32 height, UINT32 depth, UINT8 format, UINT32 flags)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;
                DXGI_FORMAT eNewFormat = gs_aFormats[format];
                if (eNewFormat != DXGI_FORMAT_UNKNOWN)
                {
                    UINT32 nBindFlags = 0;
                    if (flags & Gek::Video3D::TEXTURE::RESOURCE)
                    {
                        nBindFlags |= D3D11_BIND_SHADER_RESOURCE;
                    }

                    if (flags & Gek::Video3D::TEXTURE::UNORDERED_ACCESS)
                    {
                        nBindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                    }

                    CComQIPtr<ID3D11Resource> spResource;
                    if (depth == 1)
                    {
                        D3D11_TEXTURE2D_DESC textureDescription;
                        textureDescription.Width = width;
                        textureDescription.Height = height;
                        textureDescription.MipLevels = 1;
                        textureDescription.Format = eNewFormat;
                        textureDescription.ArraySize = 1;
                        textureDescription.SampleDesc.Count = 1;
                        textureDescription.SampleDesc.Quality = 0;
                        textureDescription.Usage = D3D11_USAGE_DEFAULT;
                        textureDescription.BindFlags = nBindFlags;
                        textureDescription.CPUAccessFlags = 0;
                        textureDescription.MiscFlags = 0;

                        CComPtr<ID3D11Texture2D> texture2D;
                        device->CreateTexture2D(&textureDescription, nullptr, &texture2D);
                        if (texture2D)
                        {
                            spResource = texture2D;
                        }
                    }
                    else
                    {
                        D3D11_TEXTURE3D_DESC textureDescription;
                        textureDescription.Width = width;
                        textureDescription.Height = height;
                        textureDescription.Depth = depth;
                        textureDescription.MipLevels = 1;
                        textureDescription.Format = eNewFormat;
                        textureDescription.Usage = D3D11_USAGE_DEFAULT;
                        textureDescription.BindFlags = nBindFlags;
                        textureDescription.CPUAccessFlags = 0;
                        textureDescription.MiscFlags = 0;

                        CComPtr<ID3D11Texture3D> texture2D;
                        device->CreateTexture3D(&textureDescription, nullptr, &texture2D);
                        if (texture2D)
                        {
                            spResource = texture2D;
                        }
                    }

                    if (spResource)
                    {
                        CComPtr<ID3D11ShaderResourceView> spResourceView;
                        if (flags & Gek::Video3D::TEXTURE::RESOURCE)
                        {
                            device->CreateShaderResourceView(spResource, nullptr, &spResourceView);
                        }

                        CComPtr<ID3D11UnorderedAccessView> spUnderedView;
                        if (flags & Gek::Video3D::TEXTURE::UNORDERED_ACCESS)
                        {
                            D3D11_UNORDERED_ACCESS_VIEW_DESC kViewDesc;
                            kViewDesc.Format = eNewFormat;
                            if (depth == 1)
                            {
                                kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                                kViewDesc.Texture2D.MipSlice = 0;
                            }
                            else
                            {
                                kViewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                                kViewDesc.Texture3D.MipSlice = 0;
                                kViewDesc.Texture3D.FirstWSlice = 0;
                                kViewDesc.Texture3D.WSize = depth;
                            }

                            device->CreateUnorderedAccessView(spResource, &kViewDesc, &spUnderedView);
                        }

                        CComPtr<Texture> texture2D(new Texture(spResourceView, spUnderedView));
                        if (texture2D)
                        {
                            resourceHandle = InterlockedIncrement(&nextResourceHandle);
                            resourceList.insert(std::make_pair(resourceHandle, RESOURCE(texture2D)));
                        }
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) loadTexture(LPCWSTR fileName, UINT32 flags)
            {
                REQUIRE_RETURN(device, Gek::InvalidHandle);
                REQUIRE_RETURN(fileName, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                std::vector<UINT8> aBuffer;
                if (SUCCEEDED(GEKLoadFromFile(fileName, aBuffer)))
                {
                    DirectX::ScratchImage kImage;
                    DirectX::TexMetadata kMetadata;
                    if (FAILED(DirectX::LoadFromDDSMemory(aBuffer.data(), aBuffer.size(), 0, &kMetadata, kImage)))
                    {
                        if (FAILED(DirectX::LoadFromTGAMemory(aBuffer.data(), aBuffer.size(), &kMetadata, kImage)))
                        {
                            DWORD aFormats[] =
                            {
                                DirectX::WIC_CODEC_PNG,              // Portable Network Graphics (.png)
                                DirectX::WIC_CODEC_BMP,              // Windows Bitmap (.bmp)
                                DirectX::WIC_CODEC_JPEG,             // Joint Photographic Experts Group (.jpg, .jpeg)
                            };

                            for (UINT32 nFormat = 0; nFormat < _ARRAYSIZE(aFormats); nFormat++)
                            {
                                if (SUCCEEDED(DirectX::LoadFromWICMemory(aBuffer.data(), aBuffer.size(), aFormats[nFormat], &kMetadata, kImage)))
                                {
                                    break;
                                }
                            }
                        }
                    }

                    CComPtr<ID3D11ShaderResourceView> spResourceView;
                    DirectX::CreateShaderResourceView(device, kImage.GetImages(), kImage.GetImageCount(), kMetadata, &spResourceView);
                    if (spResourceView)
                    {
                        CComPtr<ID3D11Resource> spResource;
                        spResourceView->GetResource(&spResource);
                        if (spResource)
                        {
                            D3D11_SHADER_RESOURCE_VIEW_DESC kViewDesc;
                            spResourceView->GetDesc(&kViewDesc);

                            UINT32 width = 1;
                            UINT32 height = 1;
                            UINT32 depth = 1;
                            if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE1D)
                            {
                                CComQIPtr<ID3D11Texture1D> spTexture1D(spResource);
                                if (spTexture1D)
                                {
                                    D3D11_TEXTURE1D_DESC description;
                                    spTexture1D->GetDesc(&description);
                                    width = description.Width;
                                }
                            }
                            else if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
                            {
                                CComQIPtr<ID3D11Texture2D> spTexture2D(spResource);
                                if (spTexture2D)
                                {
                                    D3D11_TEXTURE2D_DESC description;
                                    spTexture2D->GetDesc(&description);
                                    width = description.Width;
                                    height = description.Height;
                                }
                            }
                            else if (kViewDesc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE3D)
                            {
                                CComQIPtr<ID3D11Texture3D> spTexture3D(spResource);
                                if (spTexture3D)
                                {
                                    D3D11_TEXTURE3D_DESC description;
                                    spTexture3D->GetDesc(&description);
                                    width = description.Width;
                                    height = description.Height;
                                    depth = description.Width;
                                }
                            }

                            CComPtr<Texture> texture2D(new Texture(spResourceView, nullptr));
                            if (texture2D)
                            {
                                resourceHandle = InterlockedIncrement(&nextResourceHandle);
                                resourceList.insert(std::make_pair(resourceHandle, RESOURCE(texture2D)));
                            }
                        }
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(void) UpdateTexture(Handle resourceHandle, void *pBuffer, UINT32 nPitch, Rectangle<UINT32> *pDestRect)
            {
                REQUIRE_VOID_RETURN(deviceContext);

                CComQIPtr<ID3D11ShaderResourceView> spRenderTargetView(GetResource(resourceHandle));
                if (spRenderTargetView)
                {
                    CComQIPtr<ID3D11Resource> spResource;
                    spRenderTargetView->GetResource(&spResource);
                    if (spResource)
                    {
                        if (pDestRect == nullptr)
                        {
                            deviceContext->UpdateSubresource(spResource, 0, nullptr, pBuffer, nPitch, nPitch);
                        }
                        else
                        {
                            D3D11_BOX nBox =
                            {
                                pDestRect->left,
                                pDestRect->top,
                                0,
                                pDestRect->right,
                                pDestRect->bottom,
                                1,
                            };

                            deviceContext->UpdateSubresource(spResource, 0, &nBox, pBuffer, nPitch, nPitch);
                        }
                    }
                }
            }

            STDMETHODIMP_(void) clearDefaultRenderTarget(const Math::Float4 &color)
            {
                REQUIRE_VOID_RETURN(deviceContext && defaultRenderTargetView);
                deviceContext->ClearRenderTargetView(defaultRenderTargetView, color.rgba);
            }

            STDMETHODIMP_(void) clearDefaultDepthStencilTarget(UINT32 flags, float depth, UINT32 stencil)
            {
                REQUIRE_VOID_RETURN(deviceContext && defaultDepthStencilView);
                deviceContext->ClearDepthStencilView(defaultDepthStencilView,
                    ((flags & Gek::Video3D::CLEAR::DEPTH ? D3D11_CLEAR_DEPTH : 0) |
                    (flags & Gek::Video3D::CLEAR::STENCIL ? D3D11_CLEAR_STENCIL : 0)),
                    depth, stencil);
            }

            STDMETHODIMP_(void) SetDefaultTargets(ContextInterface *deviceContext, Handle depthHandle)
            {
                REQUIRE_VOID_RETURN(deviceContext || deviceContext);
                REQUIRE_VOID_RETURN(defaultRenderTargetView);
                REQUIRE_VOID_RETURN(defaultDepthStencilView);

                D3D11_VIEWPORT kViewport;
                kViewport.TopLeftX = 0.0f;
                kViewport.TopLeftY = 0.0f;
                kViewport.Width = float(width);
                kViewport.Height = float(height);
                kViewport.MinDepth = 0.0f;
                kViewport.MaxDepth = 1.0f;
                IUnknown *pDepth = GetResource(depthHandle);
                ID3D11RenderTargetView *pRenderTargetView = defaultRenderTargetView;
                CComQIPtr<ID3D11DepthStencilView> spDepth(pDepth ? pDepth : defaultDepthStencilView);
                CComQIPtr<ID3D11DeviceContext> sdeviceContext(deviceContext);
                if (sdeviceContext)
                {
                    sdeviceContext->OMSetRenderTargets(1, &pRenderTargetView, spDepth);
                    sdeviceContext->RSSetViewports(1, &kViewport);
                }
                else
                {
                    deviceContext->OMSetRenderTargets(1, &pRenderTargetView, spDepth);
                    deviceContext->RSSetViewports(1, &kViewport);
                }
            }

            STDMETHODIMP_(void) ExecuteCommandList(IUnknown *instance)
            {
                REQUIRE_VOID_RETURN(deviceContext && instance);

                CComQIPtr<ID3D11CommandList> commandList(instance);
                if (commandList)
                {
                    deviceContext->ExecuteCommandList(commandList, FALSE);
                }
            }

            STDMETHODIMP_(void) Present(bool bWaitForVSync)
            {
                REQUIRE_VOID_RETURN(swapChain);

                swapChain->Present(bWaitForVSync ? 1 : 0, 0);
            }

            // Video2D::SystemInterface
            STDMETHODIMP_(Gek::Handle) createBrush(const Math::Float4 &nColor)
            {
                REQUIRE_RETURN(deviceContext2D, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID2D1SolidColorBrush> spBrush;
                deviceContext2D->CreateSolidColorBrush(*(D2D1_COLOR_F *)&nColor, &spBrush);
                if (spBrush)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spBrush)));
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createBrush(const std::vector<GEK2DVIDEO::GRADIENT::STOP> &aStops, const Rectangle<float> &kRect)
            {
                REQUIRE_RETURN(deviceContext2D, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                CComPtr<ID2D1GradientStopCollection> spStops;
                deviceContext2D->CreateGradientStopCollection((D2D1_GRADIENT_STOP *)aStops.data(), aStops.size(), &spStops);
                if (spStops)
                {
                    CComPtr<ID2D1LinearGradientBrush> spBrush;
                    deviceContext2D->CreateLinearGradientBrush(*(D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *)&kRect, spStops, &spBrush);
                    if (spBrush)
                    {
                        resourceHandle = InterlockedIncrement(&nextResourceHandle);
                        resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spBrush)));
                    }
                }

                return resourceHandle;
            }

            STDMETHODIMP_(Gek::Handle) createFont(LPCWSTR pFace, UINT32 nWeight, GEK2DVIDEO::FONT::STYLE eStyle, float nSize)
            {
                REQUIRE_RETURN(deviceContext2D, Gek::InvalidHandle);
                REQUIRE_RETURN(pFace, Gek::InvalidHandle);

                Gek::Handle resourceHandle = Gek::InvalidHandle;

                DWRITE_FONT_WEIGHT eD2DWeight = DWRITE_FONT_WEIGHT(nWeight);
                DWRITE_FONT_STYLE eD2DStyle = gs_aFontStyles[eStyle];

                CComPtr<IDWriteTextFormat> spFormat;
                factoryWrite->CreateTextFormat(pFace, nullptr, eD2DWeight, eD2DStyle, DWRITE_FONT_STRETCH_NORMAL, nSize, L"", &spFormat);
                if (spFormat)
                {
                    resourceHandle = InterlockedIncrement(&nextResourceHandle);
                    resourceList.insert(std::make_pair(resourceHandle, RESOURCE(spFormat)));
                }

                return resourceHandle;
            }

            STDMETHODIMP CreateGeometry(GeometryInterface **ppGeometry)
            {
                REQUIRE_RETURN(factoryWrite, E_FAIL);
                REQUIRE_RETURN(ppGeometry, E_INVALIDARG);

                CComPtr<ID2D1PathGeometry> spPathGeometry;
                HRESULT resultValue = factory2D->CreatePathGeometry(&spPathGeometry);
                if (spPathGeometry)
                {
                    resultValue = E_OUTOFMEMORY;
                    CComPtr<Geometry> spGeometry(new Geometry(spPathGeometry));
                    if (spGeometry)
                    {
                        resultValue = spGeometry->QueryInterface(IID_PPV_ARGS(ppGeometry));
                    }
                }

                return resultValue;
            }

            STDMETHODIMP_(void) SetTransform(const Math::Float3x2 &nTransform)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);

                deviceContext2D->SetTransform(*(D2D1_MATRIX_3X2_F *)&nTransform);
            }

            STDMETHODIMP_(void) DrawText(const Rectangle<float> &kLayout, Handle nFontID, Handle nBrushID, LPCWSTR pMessage, ...)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);
                REQUIRE_VOID_RETURN(pMessage);

                CStringW strMessage;

                va_list pArgs;
                va_start(pArgs, pMessage);
                strMessage.AppendFormatV(pMessage, pArgs);
                va_end(pArgs);

                if (!strMessage.IsEmpty())
                {
                    CComQIPtr<IDWriteTextFormat> spFormat(GetResource(nFontID));
                    CComQIPtr<ID2D1SolidColorBrush> spBrush(GetResource(nBrushID));
                    if (spFormat && spBrush)
                    {
                        deviceContext2D->DrawText(strMessage, strMessage.GetLength(), spFormat, *(D2D1_RECT_F *)&kLayout, spBrush);
                    }
                }
            }

            STDMETHODIMP_(void) DrawRectangle(const Rectangle<float> &kRect, Handle nBrushID, bool fillShape)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);

                CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
                if (spBrush)
                {
                    if (fillShape)
                    {
                        deviceContext2D->FillRectangle(*(D2D1_RECT_F *)&kRect, spBrush);
                    }
                    else
                    {
                        deviceContext2D->DrawRectangle(*(D2D1_RECT_F *)&kRect, spBrush);
                    }
                }
            }

            STDMETHODIMP_(void) DrawRectangle(const Rectangle<float> &kRect, const Math::Float2 &nRadius, Handle nBrushID, bool fillShape)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);

                CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
                if (spBrush)
                {
                    if (fillShape)
                    {
                        deviceContext2D->FillRoundedRectangle({ *(D2D1_RECT_F *)&kRect, nRadius.x, nRadius.y }, spBrush);
                    }
                    else
                    {
                        deviceContext2D->DrawRoundedRectangle({ *(D2D1_RECT_F *)&kRect, nRadius.x, nRadius.y }, spBrush);
                    }
                }
            }

            STDMETHODIMP_(void) DrawGeometry(GeometryInterface *pGeometry, Handle nBrushID, bool fillShape)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);
                REQUIRE_VOID_RETURN(pGeometry);

                CComQIPtr<ID2D1Brush> spBrush(GetResource(nBrushID));
                CComQIPtr<ID2D1PathGeometry> spGeometry(pGeometry);
                if (spBrush && spGeometry)
                {
                    if (fillShape)
                    {
                        deviceContext2D->FillGeometry(spGeometry, spBrush);
                    }
                    else
                    {
                        deviceContext2D->DrawGeometry(spGeometry, spBrush);
                    }
                }
            }

            STDMETHODIMP_(void) Begin(void)
            {
                REQUIRE_VOID_RETURN(deviceContext2D);
                deviceContext2D->BeginDraw();
            }

            STDMETHODIMP End(void)
            {
                REQUIRE_RETURN(deviceContext2D, E_FAIL);
                return deviceContext2D->EndDraw();
            }
        };

        REGISTER_CLASS(System);
    }; // namespace Video
}; // namespace Gek
