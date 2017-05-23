#pragma warning(disable : 4005)

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Render/Device.hpp"
#include "GEK/Render/Window.hpp"
#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <wincodec.h>
#include <algorithm>
#include <memory>
#include <ppl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
    namespace DirectX
    {
        // All these lists must match, since the same GEK Format can be used for either textures or buffers
        // The size list must also match
        static const DXGI_FORMAT TextureFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R16_FLOAT,

            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R8_UINT,

            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_SINT,

            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R8_UNORM,

            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R8_SNORM,

            DXGI_FORMAT_R32G8X24_TYPELESS,
            DXGI_FORMAT_R24G8_TYPELESS,

            DXGI_FORMAT_R32_TYPELESS,
            DXGI_FORMAT_R16_TYPELESS,
        };

        static_assert(ARRAYSIZE(TextureFormatList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all TextureFormatList.");

        static const DXGI_FORMAT DepthFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
            DXGI_FORMAT_D24_UNORM_S8_UINT,

            DXGI_FORMAT_D32_FLOAT,
            DXGI_FORMAT_D16_UNORM,
        };

        static_assert(ARRAYSIZE(DepthFormatList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all DepthFormatList.");

        static const DXGI_FORMAT ViewFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R16_FLOAT,

            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R8_UINT,

            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_SINT,

            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R8_UNORM,

            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R8_SNORM,

            DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            DXGI_FORMAT_R24_UNORM_X8_TYPELESS,

            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R16_UNORM,
        };

        static_assert(ARRAYSIZE(ViewFormatList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all ViewFormatList.");

        static const DXGI_FORMAT BufferFormatList[] =
        {
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_R32G32B32A32_FLOAT,
            DXGI_FORMAT_R16G16B16A16_FLOAT,
            DXGI_FORMAT_R32G32B32_FLOAT,
            DXGI_FORMAT_R11G11B10_FLOAT,
            DXGI_FORMAT_R32G32_FLOAT,
            DXGI_FORMAT_R16G16_FLOAT,
            DXGI_FORMAT_R32_FLOAT,
            DXGI_FORMAT_R16_FLOAT,

            DXGI_FORMAT_R32G32B32A32_UINT,
            DXGI_FORMAT_R16G16B16A16_UINT,
            DXGI_FORMAT_R10G10B10A2_UINT,
            DXGI_FORMAT_R8G8B8A8_UINT,
            DXGI_FORMAT_R32G32B32_UINT,
            DXGI_FORMAT_R32G32_UINT,
            DXGI_FORMAT_R16G16_UINT,
            DXGI_FORMAT_R8G8_UINT,
            DXGI_FORMAT_R32_UINT,
            DXGI_FORMAT_R16_UINT,
            DXGI_FORMAT_R8_UINT,

            DXGI_FORMAT_R32G32B32A32_SINT,
            DXGI_FORMAT_R16G16B16A16_SINT,
            DXGI_FORMAT_R8G8B8A8_SINT,
            DXGI_FORMAT_R32G32B32_SINT,
            DXGI_FORMAT_R32G32_SINT,
            DXGI_FORMAT_R16G16_SINT,
            DXGI_FORMAT_R8G8_SINT,
            DXGI_FORMAT_R32_SINT,
            DXGI_FORMAT_R16_SINT,
            DXGI_FORMAT_R8_SINT,

            DXGI_FORMAT_R16G16B16A16_UNORM,
            DXGI_FORMAT_R10G10B10A2_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_R16G16_UNORM,
            DXGI_FORMAT_R8G8_UNORM,
            DXGI_FORMAT_R16_UNORM,
            DXGI_FORMAT_R8_UNORM,

            DXGI_FORMAT_R16G16B16A16_SNORM,
            DXGI_FORMAT_R8G8B8A8_SNORM,
            DXGI_FORMAT_R16G16_SNORM,
            DXGI_FORMAT_R8G8_SNORM,
            DXGI_FORMAT_R16_SNORM,
            DXGI_FORMAT_R8_SNORM,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,

            DXGI_FORMAT_UNKNOWN,
            DXGI_FORMAT_UNKNOWN,
        };

        static_assert(ARRAYSIZE(BufferFormatList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all BufferFormatList.");

        static const uint32_t FormatStrideList[] =
        {
            0, // DXGI_FORMAT_UNKNOWN,

            (sizeof(uint32_t) * 4), // DXGI_FORMAT_R32G32B32A32_FLOAT,
            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_FLOAT,
            (sizeof(uint32_t) * 3), // DXGI_FORMAT_R32G32B32_FLOAT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R11G11B10_FLOAT,
            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32G32_FLOAT,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_FLOAT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_FLOAT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_FLOAT,

            (sizeof(uint32_t) * 4), // DXGI_FORMAT_R32G32B32A32_UINT,
            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_UINT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R10G10B10A2_UINT,
            (sizeof(uint8_t) * 4), // DXGI_FORMAT_R8G8B8A8_UINT,
            (sizeof(uint32_t) * 3), // DXGI_FORMAT_R32G32B32_UINT,
            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32G32_UINT,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_UINT,
            (sizeof(uint8_t) * 2), // DXGI_FORMAT_R8G8_UINT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_UINT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UINT,
            (sizeof(uint8_t) * 1), // DXGI_FORMAT_R8_UINT,

            (sizeof(uint32_t) * 4), // DXGI_FORMAT_R32G32B32A32_SINT,
            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_SINT,
            (sizeof(uint8_t) * 4), // DXGI_FORMAT_R8G8B8A8_SINT,
            (sizeof(uint32_t) * 3), // DXGI_FORMAT_R32G32B32_SINT,
            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32G32_SINT,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_SINT,
            (sizeof(uint8_t) * 2), // DXGI_FORMAT_R8G8_SINT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_SINT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_SINT,
            (sizeof(uint8_t) * 1), // DXGI_FORMAT_R8_SINT,

            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_UNORM,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R10G10B10A2_UNORM,
            (sizeof(uint8_t) * 4), // DXGI_FORMAT_R8G8B8A8_UNORM,
            (sizeof(uint8_t) * 4), // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_UNORM,
            (sizeof(uint8_t) * 2), // DXGI_FORMAT_R8G8_UNORM,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UNORM,
            (sizeof(uint8_t) * 1), // DXGI_FORMAT_R8_UNORM,

            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_SNORM,
            (sizeof(uint8_t) * 4), // DXGI_FORMAT_R8G8B8A8_SNORM,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_SNORM,
            (sizeof(uint8_t) * 2), // DXGI_FORMAT_R8G8_SNORM,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_SNORM,
            (sizeof(uint8_t) * 1), // DXGI_FORMAT_R8_SNORM,

            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R24_UNORM_X8_TYPELESS,

            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_FLOAT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UNORM,
        };

        static_assert(ARRAYSIZE(FormatStrideList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all FormatStrideList.");

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
            D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
            D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
            D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D11_FILTER_COMPARISON_ANISOTROPIC,
            D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
            D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
            D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
            D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
            D3D11_FILTER_MINIMUM_ANISOTROPIC,
            D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
            D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
            D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
            D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
            D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
            D3D11_FILTER_MAXIMUM_ANISOTROPIC,
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

        static char const * const VertexSemanticList[] =
        {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMA",
            "NORMA",
            "COLOR",
        };

        static char const * const PixelSemanticList[] =
        {
            "SV_POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMA",
            "NORMA",
            "COLOR",
        };

        static_assert(ARRAYSIZE(VertexSemanticList) == static_cast<uint8_t>(Render::ElementDeclaration::Semantic::Count), "New element semantic added without adding to all VertexSemanticList.");
        static_assert(ARRAYSIZE(PixelSemanticList) == static_cast<uint8_t>(Render::ElementDeclaration::Semantic::Count), "New element semantic added without adding to all PixelSemanticList.");

        Render::Format getFormat(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return Render::Format::R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return Render::Format::R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32B32_FLOAT: return Render::Format::R32G32B32_FLOAT;
            case DXGI_FORMAT_R11G11B10_FLOAT: return Render::Format::R11G11B10_FLOAT;
            case DXGI_FORMAT_R32G32_FLOAT: return Render::Format::R32G32_FLOAT;
            case DXGI_FORMAT_R16G16_FLOAT: return Render::Format::R16G16_FLOAT;
            case DXGI_FORMAT_R32_FLOAT: return Render::Format::R32_FLOAT;
            case DXGI_FORMAT_R16_FLOAT: return Render::Format::R16_FLOAT;

            case DXGI_FORMAT_R32G32B32A32_UINT: return Render::Format::R32G32B32A32_UINT;
            case DXGI_FORMAT_R16G16B16A16_UINT: return Render::Format::R16G16B16A16_UINT;
            case DXGI_FORMAT_R10G10B10A2_UINT: return Render::Format::R10G10B10A2_UINT;
            case DXGI_FORMAT_R8G8B8A8_UINT: return Render::Format::R8G8B8A8_UINT;
            case DXGI_FORMAT_R32G32B32_UINT: return Render::Format::R32G32B32_UINT;
            case DXGI_FORMAT_R32G32_UINT: return Render::Format::R32G32_UINT;
            case DXGI_FORMAT_R16G16_UINT: return Render::Format::R16G16_UINT;
            case DXGI_FORMAT_R8G8_UINT: return Render::Format::R8G8_UINT;
            case DXGI_FORMAT_R32_UINT: return Render::Format::R32_UINT;
            case DXGI_FORMAT_R16_UINT: return Render::Format::R16_UINT;
            case DXGI_FORMAT_R8_UINT: return Render::Format::R8_UINT;

            case DXGI_FORMAT_R32G32B32A32_SINT: return Render::Format::R32G32B32A32_INT;
            case DXGI_FORMAT_R16G16B16A16_SINT: return Render::Format::R16G16B16A16_INT;
            case DXGI_FORMAT_R8G8B8A8_SINT: return Render::Format::R8G8B8A8_INT;
            case DXGI_FORMAT_R32G32B32_SINT: return Render::Format::R32G32B32_INT;
            case DXGI_FORMAT_R32G32_SINT: return Render::Format::R32G32_INT;
            case DXGI_FORMAT_R16G16_SINT: return Render::Format::R16G16_INT;
            case DXGI_FORMAT_R8G8_SINT: return Render::Format::R8G8_INT;
            case DXGI_FORMAT_R32_SINT: return Render::Format::R32_INT;
            case DXGI_FORMAT_R16_SINT: return Render::Format::R16_INT;
            case DXGI_FORMAT_R8_SINT: return Render::Format::R8_INT;

            case DXGI_FORMAT_R16G16B16A16_UNORM: return Render::Format::R16G16B16A16_UNORM;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return Render::Format::R10G10B10A2_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return Render::Format::R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Render::Format::R8G8B8A8_UNORM_SRGB;
            case DXGI_FORMAT_R16G16_UNORM: return Render::Format::R16G16_UNORM;
            case DXGI_FORMAT_R8G8_UNORM: return Render::Format::R8G8_UNORM;
            case DXGI_FORMAT_R16_UNORM: return Render::Format::R16_UNORM;
            case DXGI_FORMAT_R8_UNORM: return Render::Format::R8_UNORM;

            case DXGI_FORMAT_R16G16B16A16_SNORM: return Render::Format::R16G16B16A16_NORM;
            case DXGI_FORMAT_R8G8B8A8_SNORM: return Render::Format::R8G8B8A8_NORM;
            case DXGI_FORMAT_R16G16_SNORM: return Render::Format::R16G16_NORM;
            case DXGI_FORMAT_R8G8_SNORM: return Render::Format::R8G8_NORM;
            case DXGI_FORMAT_R16_SNORM: return Render::Format::R16_NORM;
            case DXGI_FORMAT_R8_SNORM: return Render::Format::R8_NORM;
            };

            return Render::Format::Unknown;
        }

        std::string getFormatSemantic(Render::Format format)
        {
            switch (format)
            {
            case Render::Format::R32G32B32A32_FLOAT:
            case Render::Format::R16G16B16A16_FLOAT:
            case Render::Format::R16G16B16A16_UNORM:
            case Render::Format::R10G10B10A2_UNORM:
            case Render::Format::R8G8B8A8_UNORM:
            case Render::Format::R8G8B8A8_UNORM_SRGB:
            case Render::Format::R16G16B16A16_NORM:
            case Render::Format::R8G8B8A8_NORM:
                return "float4";

            case Render::Format::R32G32B32_FLOAT:
            case Render::Format::R11G11B10_FLOAT:
                return "float3";

            case Render::Format::R32G32_FLOAT:
            case Render::Format::R16G16_FLOAT:
            case Render::Format::R16G16_UNORM:
            case Render::Format::R8G8_UNORM:
            case Render::Format::R16G16_NORM:
            case Render::Format::R8G8_NORM:
                return "float2";

            case Render::Format::R32_FLOAT:
            case Render::Format::R16_FLOAT:
            case Render::Format::R16_UNORM:
            case Render::Format::R8_UNORM:
            case Render::Format::R16_NORM:
            case Render::Format::R8_NORM:
            case Render::Format::D32_FLOAT_S8X24_UINT:
            case Render::Format::D24_UNORM_S8_UINT:
            case Render::Format::D32_FLOAT:
            case Render::Format::D16_UNORM:
                return "float";

            case Render::Format::R32G32B32A32_UINT:
            case Render::Format::R16G16B16A16_UINT:
            case Render::Format::R10G10B10A2_UINT:
                return "uint4";

            case Render::Format::R8G8B8A8_UINT:
            case Render::Format::R32G32B32_UINT:
                return "uint3";

            case Render::Format::R32G32_UINT:
            case Render::Format::R16G16_UINT:
            case Render::Format::R8G8_UINT:
                return "uint2";

            case Render::Format::R32_UINT:
            case Render::Format::R16_UINT:
            case Render::Format::R8_UINT:
                return "uint";

            case Render::Format::R32G32B32A32_INT:
            case Render::Format::R16G16B16A16_INT:
            case Render::Format::R8G8B8A8_INT:
                return "int4";

            case Render::Format::R32G32B32_INT:
                return "int3";

            case Render::Format::R32G32_INT:
            case Render::Format::R16G16_INT:
            case Render::Format::R8G8_INT:
                return "int2";

            case Render::Format::R32_INT:
            case Render::Format::R16_INT:
            case Render::Format::R8_INT:
                return "int";
            };

			return String::Empty;
        }
    }; // namespace DirectX

    namespace Direct3D11
    {
        template <typename CLASS>
        void setDebugName(CComPtr<CLASS> &object, std::string const &name, std::string const &member = std::string())
        {
			auto finalName(name + (member.empty() ? "::" : String::Empty) + (member.empty() ? member : String::Empty));
            object->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(finalName.size()), finalName.c_str());
        }

        template <typename HANDLE, typename TYPE>
        class DataCache
        {
        private:
            std::unordered_map<HANDLE, CComPtr<TYPE>> dataMap;

        public:
            void clear(void)
            {
                dataMap.clear();
            }

            void remove(HANDLE handle)
            {
                dataMap.erase(handle);
            }

            void set(HANDLE handle, CComPtr<TYPE> &data)
            {
                dataMap[handle] = data;
            }

            TYPE * get(HANDLE handle)
            {
                auto dataSearch = (handle ? dataMap.find(handle) : std::end(dataMap));
                return (dataSearch == std::end(dataMap) ? nullptr : dataSearch->second.p);
            }
        };

        template <typename HANDLE, typename TYPE>
        class HashCache
            : public DataCache<HANDLE, TYPE>
        {
        private:
            uint32_t nextHandle = 0;
            std::unordered_map<size_t, HANDLE> hashMap;

        public:
            void remove(HANDLE handle)
            {
                DataCache::remove(handle);
            }

            HANDLE insert(size_t hash, std::function<CComPtr<TYPE>(HANDLE)> &&onLoadRequired)
            {
                auto hashSearch = hashMap.find(hash);
                if (hashSearch != std::end(hashMap))
                {
                    return hashSearch->second;
                }

                auto handle(InterlockedIncrement(&nextHandle));
                hashMap[hash] = handle;
                set(handle, onLoadRequired(handle));
                return handle;
            }
        };

        template <typename HANDLE, typename TYPE>
        class UniqueCache
            : public DataCache<HANDLE, TYPE>
        {
        private:
            uint32_t nextHandle = 0;

        public:
            HANDLE insert(CComPtr<TYPE> &data)
            {
                auto handle(InterlockedIncrement(&nextHandle));
                set(handle, data);
                return handle;
            }
        };

        template <typename HANDLE, typename TYPE>
        struct FunctionCache
        {
            DataCache<HANDLE, TYPE> &dataCache;
            std::vector<TYPE *> objectList;

            FunctionCache(DataCache<HANDLE, TYPE> &dataCache)
                : dataCache(dataCache)
            {
            }

            TYPE * const * const update(const std::vector<HANDLE> &inputList)
            {
                objectList.clear();
                objectList.reserve(std::max(inputList.size(), objectList.size()));
                for(auto &handle : inputList)
                {
                    objectList.push_back(dataCache.get(handle));
                }

                return objectList.data();
            }
        };

        GEK_CONTEXT_USER(Device, Window *, Render::Device::Description)
            , public Render::Debug::Device
        {
            struct PipelineState
                : public IUnknown
            {
                CComPtr<ID3D11RasterizerState> rasterizerState;
                CComPtr<ID3D11DepthStencilState> depthStencilState;
                CComPtr<ID3D11BlendState> blendState;
                CComPtr<ID3D11InputLayout> vertexDeclaration;
                CComPtr<ID3D11VertexShader> vertexShader;
                CComPtr<ID3D11PixelShader> pixelShader;
                D3D11_PRIMITIVE_TOPOLOGY primitiveTopology;
                uint32_t sampleMask = 0x0;

                uint32_t referenceCount = 0;
                STDMETHODIMP QueryInterface(REFIID interfaceID, void **returnObject)
                {
                    if (IsEqualIID(interfaceID, IID_IUnknown))
                    {
                        (*returnObject) = static_cast<IUnknown *>(this);
                        return S_OK;
                    }

                    return E_NOINTERFACE;
                }

                STDMETHODIMP_(ULONG) AddRef(void)
                {
                    return InterlockedIncrement(&referenceCount);
                }

                STDMETHODIMP_(ULONG) Release(void)
                {
                    auto returnValue = InterlockedDecrement(&referenceCount);
                    if (returnValue == 0)
                    {
                        delete this;
                    }

                    return returnValue;
                }
            };

            class Queue
                : public Render::Device::Queue
            {
            public:
                Device *device = nullptr;
                CComPtr<ID3D11DeviceContext> d3dDeviceContext;
                FunctionCache<Render::SamplerStateHandle, ID3D11SamplerState> samplerStateCache;
                FunctionCache<Render::ResourceHandle, ID3D11Buffer> vertexConstantBufferCache;
                FunctionCache<Render::ResourceHandle, ID3D11Buffer> pixelConstantBufferCache;
                FunctionCache<Render::ResourceHandle, ID3D11ShaderResourceView> vertexResourceCache;
                FunctionCache<Render::ResourceHandle, ID3D11ShaderResourceView> pixelResourceCache;
                FunctionCache<Render::ResourceHandle, ID3D11UnorderedAccessView> unorderedAccessCache;
                FunctionCache<Render::ResourceHandle, ID3D11Buffer> vertexBufferCache;
                FunctionCache<Render::ResourceHandle, ID3D11RenderTargetView> renderTargetCache;
                std::array<std::vector<uint32_t>, 2> vertexDataCache;

            public:
                Queue(Device *device, CComPtr<ID3D11DeviceContext> &d3dDeviceContext)
                    : device(device)
                    , d3dDeviceContext(d3dDeviceContext)
                    , samplerStateCache(device->samplerStateCache)
                    , vertexConstantBufferCache(device->bufferCache)
                    , pixelConstantBufferCache(device->bufferCache)
                    , vertexResourceCache(device->shaderResourceViewCache)
                    , pixelResourceCache(device->shaderResourceViewCache)
                    , unorderedAccessCache(device->unorderedAccessViewCache)
                    , vertexBufferCache(device->bufferCache)
                    , renderTargetCache(device->renderTargetViewCache)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                }

                // Render::Device::Queue
                void reset(void)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    CComPtr<ID3D11CommandList> commandList;
                    d3dDeviceContext->FinishCommandList(false, &commandList);
                }

                void generateMipMaps(Render::ResourceHandle texture)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->GenerateMips(device->shaderResourceViewCache.get(texture));
                }

                void resolveSamples(Render::ResourceHandle destination, Render::ResourceHandle source)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ResolveSubresource(device->resourceCache.get(destination), 0, device->resourceCache.get(source), 0, DXGI_FORMAT_UNKNOWN);
                }

                void clearUnorderedAccess(Render::ResourceHandle object, Math::Float4 const &value)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearUnorderedAccessViewFloat(device->unorderedAccessViewCache.get(object), value.data);
                }

                void clearUnorderedAccess(Render::ResourceHandle object, Math::UInt4 const &value)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearUnorderedAccessViewUint(device->unorderedAccessViewCache.get(object), value.data);
                }

                void clearRenderTarget(Render::ResourceHandle renderTarget, Math::Float4 const &clearColor)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearRenderTargetView(device->renderTargetViewCache.get(renderTarget), clearColor.data);
                }

                void clearDepthStencilTarget(Render::ResourceHandle depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearDepthStencilView(device->depthStencilViewCache.get(depthBuffer),
                        ((flags & Render::ClearFlags::Depth ? D3D11_CLEAR_DEPTH : 0) |
                        (flags & Render::ClearFlags::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                        clearDepth, clearStencil);
                }

                void setViewportList(const std::vector<Render::ViewPort> &viewPortList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->RSSetViewports(viewPortList.size(), (D3D11_VIEWPORT *)viewPortList.data());
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->RSSetScissorRects(rectangleList.size(), (D3D11_RECT *)rectangleList.data());
                }

                void bindPipelineState(Render::PipelineStateHandle pipelineStateHandle)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    auto pipelineState = device->pipelineStateCache.get(pipelineStateHandle);
                    if (pipelineState)
                    {
                        d3dDeviceContext->RSSetState(pipelineState->rasterizerState);
                        d3dDeviceContext->OMSetDepthStencilState(pipelineState->depthStencilState, 0x0);
                        d3dDeviceContext->OMSetBlendState(pipelineState->blendState, Math::Float4::Zero.data, pipelineState->sampleMask);
                        d3dDeviceContext->IASetInputLayout(pipelineState->vertexDeclaration);
                        d3dDeviceContext->VSSetShader(pipelineState->vertexShader, nullptr, 0);
                        d3dDeviceContext->PSSetShader(pipelineState->pixelShader, nullptr, 0);
                        d3dDeviceContext->IASetPrimitiveTopology(pipelineState->primitiveTopology);
                    }
                }

                void bindSamplerStateList(const std::vector<Render::SamplerStateHandle> &list, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeviceContext->VSSetSamplers(firstStage, list.size(), samplerStateCache.update(list));
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeviceContext->PSSetSamplers(firstStage, list.size(), samplerStateCache.update(list));
                    }
                }

                void bindConstantBufferList(const std::vector<Render::ResourceHandle> &list, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeviceContext->VSSetConstantBuffers(firstStage, list.size(), vertexConstantBufferCache.update(list));
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeviceContext->PSSetConstantBuffers(firstStage, list.size(), pixelConstantBufferCache.update(list));
                    }
                }

                void bindResourceList(const std::vector<Render::ResourceHandle> &list, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeviceContext->VSSetShaderResources(firstStage, list.size(), vertexResourceCache.update(list));
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeviceContext->PSSetShaderResources(firstStage, list.size(), pixelResourceCache.update(list));
                    }
                }

                void bindUnorderedAccessList(const std::vector<Render::ResourceHandle> &list, uint32_t firstStage, uint32_t *countList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, list.size(), unorderedAccessCache.update(list), countList);
                }

                void bindIndexBuffer(Render::ResourceHandle indexBuffer, uint32_t offset)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    auto description = device->getBufferDescription(indexBuffer);
                    if (description)
                    {
                        DXGI_FORMAT format = DirectX::BufferFormatList[static_cast<uint8_t>(description->format)];
                        d3dDeviceContext->IASetIndexBuffer(device->bufferCache.get(indexBuffer), format, offset);
                    }
                }

                void bindVertexBufferList(const std::vector<Render::ResourceHandle> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    vertexDataCache[0].clear();
                    vertexDataCache[1].clear();
                    auto vertexBufferCount = vertexBufferList.size();
                    vertexDataCache[0].reserve(std::max(vertexBufferCount, vertexDataCache[0].size()));
                    vertexDataCache[1].reserve(std::max(vertexBufferCount, vertexDataCache[1].size()));
                    for (size_t buffer = 0; buffer < vertexBufferCount; ++buffer)
                    {
                        auto description = device->getBufferDescription(vertexBufferList[buffer]);
                        vertexDataCache[0].push_back(description ? description->stride : 0);
                        vertexDataCache[1].push_back(offsetList ? offsetList[buffer] : 0);
                    }

                    d3dDeviceContext->IASetVertexBuffers(firstSlot, vertexBufferList.size(), vertexBufferCache.update(vertexBufferList), vertexDataCache[0].data(), vertexDataCache[1].data());
                }

                void bindRenderTargetList(const std::vector<Render::ResourceHandle> &renderTargetList, Render::ResourceHandle depthBuffer)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->OMSetRenderTargets(renderTargetList.size(), renderTargetCache.update(renderTargetList), nullptr);
                }

                void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->Draw(vertexCount, firstVertex);
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }

                void drawInstancedPrimitive(Render::ResourceHandle bufferArguments)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    
                    d3dDeviceContext->DrawInstancedIndirect(device->bufferCache.get(bufferArguments), 0);
                }

                void drawInstancedIndexedPrimitive(Render::ResourceHandle bufferArguments)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    
                    d3dDeviceContext->DrawIndexedInstancedIndirect(device->bufferCache.get(bufferArguments), 0);
                }

                void dispatch(Render::ResourceHandle bufferArguments)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    
                    d3dDeviceContext->DispatchIndirect(device->bufferCache.get(bufferArguments), 0);
                }

                void runQueue(Render::QueueHandle queue)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    auto commandList = device->queueCache.get(queue);
                    if (commandList)
                    {
                        d3dDeviceContext->ExecuteCommandList(commandList, false);
                    }
                }
            };

        public:
            Window *window = nullptr;
            bool isChildWindow = false;

            CComPtr<ID3D11Device> d3dDevice;
            CComPtr<ID3D11DeviceContext> d3dDeviceContext;
            CComPtr<IDXGISwapChain1> dxgiSwapChain;

            HashCache<Render::PipelineStateHandle, PipelineState> pipelineStateCache;

            HashCache<Render::SamplerStateHandle, ID3D11SamplerState> samplerStateCache;

            HashCache<Render::ResourceHandle, ID3D11Resource> resourceCache;
            DataCache<Render::ResourceHandle, ID3D11Buffer> bufferCache;
            DataCache<Render::ResourceHandle, ID3D11ShaderResourceView> shaderResourceViewCache;
            DataCache<Render::ResourceHandle, ID3D11UnorderedAccessView> unorderedAccessViewCache;
            DataCache<Render::ResourceHandle, ID3D11RenderTargetView> renderTargetViewCache;
            DataCache<Render::ResourceHandle, ID3D11DepthStencilView> depthStencilViewCache;
            std::unordered_map<Render::ResourceHandle, Render::TextureDescription> textureDescriptionMap;
            std::unordered_map<Render::ResourceHandle, Render::BufferDescription> bufferDescriptionMap;

            UniqueCache<Render::QueueHandle, ID3D11CommandList> queueCache;

        public:
            Device(Gek::Context *context, Window *window, Render::Device::Description deviceDescription)
                : ContextRegistration(context)
                , window(window)
                , isChildWindow(GetParent((HWND)window->getBaseWindow()) != nullptr)
            {
                GEK_REQUIRE(window);

                UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
                flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

                D3D_FEATURE_LEVEL featureLevelList[] =
                {
                    D3D_FEATURE_LEVEL_11_0,
                };

                D3D_FEATURE_LEVEL featureLevel;
                HRESULT resultValue = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, 1, D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dDeviceContext);
                if (featureLevel != featureLevelList[0])
                {
                    throw Render::FeatureLevelNotSupported("Direct3D 11.0 feature level required");
                }

                if (FAILED(resultValue) || !d3dDevice || !d3dDeviceContext)
                {
                    throw Render::InitializationFailed("Unable to create rendering device and context");
                }

                CComPtr<IDXGIFactory2> dxgiFactory;
                resultValue = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
                if (FAILED(resultValue) || !dxgiFactory)
                {
                    throw Render::InitializationFailed("Unable to get graphics factory");
                }

                DXGI_SWAP_CHAIN_DESC1 swapChainDescription;
                swapChainDescription.Width = 0;
                swapChainDescription.Height = 0;
                swapChainDescription.Format = DirectX::TextureFormatList[static_cast<uint8_t>(deviceDescription.displayFormat)];
                swapChainDescription.Stereo = false;
                swapChainDescription.SampleDesc.Count = deviceDescription.sampleCount;
                swapChainDescription.SampleDesc.Quality = deviceDescription.sampleQuality;
                swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDescription.BufferCount = 2;
                swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
                swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                swapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
                resultValue = dxgiFactory->CreateSwapChainForHwnd(d3dDevice, (HWND)window->getBaseWindow(), &swapChainDescription, nullptr, nullptr, &dxgiSwapChain);
                if (FAILED(resultValue) || !dxgiSwapChain)
                {
                    throw Render::InitializationFailed("Unable to create swap chain for window");
                }

                dxgiFactory->MakeWindowAssociation((HWND)window->getBaseWindow(), 0);

#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                CComQIPtr<ID3D11InfoQueue> d3dInfoQueue(d3dDebug);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                //d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif

                updateSwapChain();
            }

            ~Device(void)
            {
                setFullScreenState(false);

                pipelineStateCache.clear();
                samplerStateCache.clear();
                resourceCache.clear();
                bufferCache.clear();
                shaderResourceViewCache.clear();
                unorderedAccessViewCache.clear();
                renderTargetViewCache.clear();
                depthStencilViewCache.clear();
                textureDescriptionMap.clear();
                bufferDescriptionMap.clear();
                queueCache.clear();

                dxgiSwapChain.Release();
                d3dDeviceContext.Release();
#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
                d3dDevice.Release();
            }

            void updateSwapChain(void)
            {
                CComPtr<ID3D11Texture2D> d3dRenderTarget;
                HRESULT resultValue = dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                if (FAILED(resultValue) || !d3dRenderTarget)
                {
                    throw Render::OperationFailed("Unable to get swap chain primary buffer");
                }

                CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                if (FAILED(resultValue) || !d3dRenderTargetView)
                {
                    throw Render::OperationFailed("Unable to create render target view for back buffer");
                }

                renderTargetViewCache.set(SwapChain, d3dRenderTargetView);

                D3D11_TEXTURE2D_DESC textureDescription;
                d3dRenderTarget->GetDesc(&textureDescription);

                Render::TextureDescription description;
                description.width = textureDescription.Width;
                description.height = textureDescription.Height;
                description.format = DirectX::getFormat(textureDescription.Format);
                textureDescriptionMap[SwapChain] = description;
            }

            CComPtr<ID3D11RasterizerState> createRasterizerState(const Render::RasterizerStateInformation &rasterizerStateInformation)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_RASTERIZER_DESC rasterizerDescription;
                rasterizerDescription.FrontCounterClockwise = rasterizerStateInformation.frontCounterClockwise;
                rasterizerDescription.DepthBias = rasterizerStateInformation.depthBias;
                rasterizerDescription.DepthBiasClamp = rasterizerStateInformation.depthBiasClamp;
                rasterizerDescription.SlopeScaledDepthBias = rasterizerStateInformation.slopeScaledDepthBias;
                rasterizerDescription.DepthClipEnable = rasterizerStateInformation.depthClipEnable;
                rasterizerDescription.ScissorEnable = rasterizerStateInformation.scissorEnable;
                rasterizerDescription.MultisampleEnable = rasterizerStateInformation.multisampleEnable;
                rasterizerDescription.AntialiasedLineEnable = rasterizerStateInformation.antialiasedLineEnable;
                rasterizerDescription.FillMode = DirectX::FillModeList[static_cast<uint8_t>(rasterizerStateInformation.fillMode)];
                rasterizerDescription.CullMode = DirectX::CullModeList[static_cast<uint8_t>(rasterizerStateInformation.cullMode)];

                CComPtr<ID3D11RasterizerState> rasterizerState;
                d3dDevice->CreateRasterizerState(&rasterizerDescription, &rasterizerState);
                if (!rasterizerState)
                {
                    throw Render::CreateObjectFailed("Unable to create rasterizer state");
                }

                return rasterizerState;
            }

            CComPtr<ID3D11DepthStencilState> createDepthState(const Render::DepthStateInformation &depthStateInformation)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = depthStateInformation.enable;
                depthStencilDescription.DepthFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthStateInformation.comparisonFunction)];
                depthStencilDescription.StencilEnable = depthStateInformation.stencilEnable;
                depthStencilDescription.StencilReadMask = depthStateInformation.stencilReadMask;
                depthStencilDescription.StencilWriteMask = depthStateInformation.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilFrontState.failOperation)];
                depthStencilDescription.FrontFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilFrontState.depthFailOperation)];
                depthStencilDescription.FrontFace.StencilPassOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilFrontState.passOperation)];
                depthStencilDescription.FrontFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthStateInformation.stencilFrontState.comparisonFunction)];
                depthStencilDescription.BackFace.StencilFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilBackState.failOperation)];
                depthStencilDescription.BackFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilBackState.depthFailOperation)];
                depthStencilDescription.BackFace.StencilPassOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthStateInformation.stencilBackState.passOperation)];
                depthStencilDescription.BackFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthStateInformation.stencilBackState.comparisonFunction)];
                depthStencilDescription.DepthWriteMask = DirectX::DepthWriteMaskList[static_cast<uint8_t>(depthStateInformation.writeMask)];

                CComPtr<ID3D11DepthStencilState> depthState;
                d3dDevice->CreateDepthStencilState(&depthStencilDescription, &depthState);
                if (!depthState)
                {
                    throw Render::CreateObjectFailed("Unable to create depth stencil state");
                }

                return depthState;
            }

            CComPtr<ID3D11BlendState> createBlendState(const Render::BlendStateInformation &blendStateInformation)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStateInformation.alphaToCoverage;
                blendDescription.IndependentBlendEnable = !blendStateInformation.unifiedBlendState;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    blendDescription.RenderTarget[renderTarget].BlendEnable = blendStateInformation.targetStateList[renderTarget].enable;
                    blendDescription.RenderTarget[renderTarget].SrcBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].colorSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].colorDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOp = DirectX::BlendOperationList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].colorOperation)];
                    blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].alphaSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].alphaDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOpAlpha = DirectX::BlendOperationList[static_cast<uint8_t>(blendStateInformation.targetStateList[renderTarget].alphaOperation)];
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                    if (blendStateInformation.targetStateList[renderTarget].writeMask & Render::BlendStateInformation::Mask::R)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (blendStateInformation.targetStateList[renderTarget].writeMask & Render::BlendStateInformation::Mask::G)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (blendStateInformation.targetStateList[renderTarget].writeMask & Render::BlendStateInformation::Mask::B)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (blendStateInformation.targetStateList[renderTarget].writeMask & Render::BlendStateInformation::Mask::A)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                CComPtr<ID3D11BlendState> blendState;
                d3dDevice->CreateBlendState(&blendDescription, &blendState);
                if (!blendState)
                {
                    throw Render::CreateObjectFailed("Unable to create blend state");
                }

                return blendState;
            }

            std::vector<D3D11_INPUT_ELEMENT_DESC> getVertexDeclaration(const std::vector<Render::VertexDeclaration> &vertexDeclaration)
            {
                uint32_t semanticIndexList[static_cast<uint8_t>(Render::ElementDeclaration::Semantic::Count)] = { 0 };
                std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescriptionList;
                for (const auto &vertexElement : vertexDeclaration)
                {
                    D3D11_INPUT_ELEMENT_DESC elementDescription;
                    elementDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(vertexElement.format)];
                    elementDescription.AlignedByteOffset = (vertexElement.alignedByteOffset == Render::VertexDeclaration::AppendAligned ? D3D11_APPEND_ALIGNED_ELEMENT : vertexElement.alignedByteOffset);
                    elementDescription.SemanticName = DirectX::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)];
                    elementDescription.SemanticIndex = semanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++;
                    elementDescription.InputSlot = vertexElement.sourceIndex;
                    switch (vertexElement.source)
                    {
                    case Render::VertexDeclaration::Source::Instance:
                        elementDescription.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        elementDescription.InstanceDataStepRate = 1;
                        break;

                    case Render::VertexDeclaration::Source::Vertex:
                    default:
                        elementDescription.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        elementDescription.InstanceDataStepRate = 0;
                        break;
                    };

                    inputElementDescriptionList.push_back(elementDescription);
                }

                return inputElementDescriptionList;
            }

            std::string getShaderHeader(const Render::PipelineStateInformation &pipelineStateInformation)
            {
                static const char ConversionFunctions[] =
                    "#define DeclareConstantBuffer(NAME, INDEX) cbuffer NAME : register(b##INDEX)\r\n" \
                    "#define DeclareSamplerState(NAME, INDEX) SamplerState NAME : register(s##INDEX)\r\n" \
                    "#define DeclareTexture1D(NAME, TYPE, INDEX) Texture1D<TYPE> NAME : register(t##INDEX)\r\n" \
                    "#define DeclareTexture2D(NAME, TYPE, INDEX) Texture2D<TYPE> NAME : register(t##INDEX)\r\n" \
                    "#define DeclareTexture3D(NAME, TYPE, INDEX) Texture3D<TYPE> NAME : register(t##INDEX)\r\n" \
                    "#define DeclareTextureCube(NAME, TYPE, INDEX) TextureCube<TYPE> NAME : register(t##INDEX)\r\n" \

                    "#define SampleTexture(TEXTURE, SAMPLER, COORD) TEXTURE.Sample(SAMPLER, COORD)\r\n" \

                    "\r\n";

                std::string shader(ConversionFunctions);
                shader.append("struct Vertex\r\n{\r\n");
                uint32_t vertexSemanticIndexList[static_cast<uint8_t>(Render::ElementDeclaration::Semantic::Count)] = { 0 };
                for (const auto &vertexElement : pipelineStateInformation.vertexDeclaration)
                {
                    std::string semantic = DirectX::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)];
                    std::string format = DirectX::getFormatSemantic(vertexElement.format);
                    shader += String::Format("    %v %v : %v%v;\r\n", format, vertexElement.name, semantic, vertexSemanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++);
                }

                shader.append("};\r\n\r\nstruct Pixel\r\n{\r\n");
                uint32_t pixelSemanticIndexList[static_cast<uint8_t>(Render::ElementDeclaration::Semantic::Count)] = { 0 };
                for (const auto &pixelElement : pipelineStateInformation.pixelDeclaration)
                {
                    std::string semantic = DirectX::PixelSemanticList[static_cast<uint8_t>(pixelElement.semantic)];
                    std::string format = DirectX::getFormatSemantic(pixelElement.format);
                    shader += String::Format("    %v %v : %v%v;\r\n", format, pixelElement.name, semantic, pixelSemanticIndexList[static_cast<uint8_t>(pixelElement.semantic)]++);
                }

                shader.append("};\r\n\r\nstruct Output\r\n{\r\n");
                uint32_t renderTargetIndex = 0;
                for (const auto &renderTarget : pipelineStateInformation.renderTargetList)
                {
                    std::string format = DirectX::getFormatSemantic(renderTarget.format);
                    shader += String::Format("    %v %v : SV_TARGET%v;\r\n", format, renderTarget.name, renderTargetIndex++);
                }

                shader.append("};\r\n\r\n");
                return shader;
            }

            std::vector<uint8_t> compileShader(std::string const &name, std::string const &type, std::string const &entryFunction, std::string const &shader, const std::string &header)
            {
                GEK_REQUIRE(d3dDevice);

                uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
                flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
                flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
                flags |= D3DCOMPILE_SKIP_VALIDATION;
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
                auto fullShader(String::Format("%v%v", header, shader));

                CComPtr<ID3DBlob> d3dShaderBlob;
                CComPtr<ID3DBlob> d3dCompilerErrors;
                HRESULT resultValue = D3DCompile(fullShader.c_str(), (fullShader.size() + 1), name.c_str(), nullptr, nullptr, entryFunction.c_str(), type.c_str(), flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
                if (FAILED(resultValue) || !d3dShaderBlob)
                {
					std::cerr << "D3DCompile Failed: " << resultValue << " " << (char const * const)d3dCompilerErrors->GetBufferPointer() << std::endl;
                    throw Render::ProgramCompilationFailed("Unable to compile shader");
                }

                uint8_t *data = (uint8_t *)d3dShaderBlob->GetBufferPointer();
                return std::vector<uint8_t>(data, (data + d3dShaderBlob->GetBufferSize()));
            }

            // Render::Debug::Device
            void * getDevice(void)
            {
                return d3dDevice.p;
            }

            // Render::Device
            Render::DisplayModeList getDisplayModeList(Render::Format format) const
            {
                Render::DisplayModeList displayModeList;

                CComPtr<IDXGIOutput> dxgiOutput;
                dxgiSwapChain->GetContainingOutput(&dxgiOutput);
                if (dxgiOutput)
                {
                    uint32_t modeCount = 0;
                    dxgiOutput->GetDisplayModeList(DirectX::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, nullptr);

                    std::vector<DXGI_MODE_DESC> dxgiDisplayModeList(modeCount);
                    dxgiOutput->GetDisplayModeList(DirectX::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, dxgiDisplayModeList.data());

                    for (const auto &dxgiDisplayMode : dxgiDisplayModeList)
                    {
                        if (dxgiDisplayMode.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE && dxgiDisplayMode.Scaling == DXGI_MODE_SCALING_CENTERED)
                        {
                            auto getAspectRatio = [](uint32_t width, uint32_t height) -> Render::DisplayMode::AspectRatio
                            {
                                const float AspectRatio4x3 = (4.0f / 3.0f);
                                const float AspectRatio16x9 = (16.0f / 9.0f);
                                const float AspectRatio16x10 = (16.0f / 10.0f);
                                float aspectRatio = (float(width) / float(height));
                                if (std::abs(aspectRatio - AspectRatio4x3) < Math::Epsilon)
                                {
                                    return Render::DisplayMode::AspectRatio::_4x3;
                                }
                                else if (std::abs(aspectRatio - AspectRatio16x9) < Math::Epsilon)
                                {
                                    return Render::DisplayMode::AspectRatio::_16x9;
                                }
                                else if (std::abs(aspectRatio - AspectRatio16x10) < Math::Epsilon)
                                {
                                    return Render::DisplayMode::AspectRatio::_16x10;
                                }
                                else
                                {
                                    return Render::DisplayMode::AspectRatio::Unknown;
                                }
                            };

                            Render::DisplayMode displayMode;
                            displayMode.width = dxgiDisplayMode.Width;
                            displayMode.height = dxgiDisplayMode.Height;
                            displayMode.format = DirectX::getFormat(dxgiDisplayMode.Format);
                            displayMode.aspectRatio = getAspectRatio(displayMode.width, displayMode.height);
                            displayMode.refreshRate.numerator = dxgiDisplayMode.RefreshRate.Numerator;
                            displayMode.refreshRate.denominator = dxgiDisplayMode.RefreshRate.Denominator;
                            displayModeList.push_back(displayMode);
                        }
                    }

                    concurrency::parallel_sort(std::begin(displayModeList), std::end(displayModeList), [](const Render::DisplayMode &left, const Render::DisplayMode &right) -> bool
                    {
                        if (left.width < right.width)
                        {
                            return true;
                        }

                        if (left.width == right.width)
                        {
                            if (left.height < right.height)
                            {
                                return true;
                            }

                            if (left.height == right.height)
                            {
                                return ((left.refreshRate.numerator / left.refreshRate.denominator) < (right.refreshRate.numerator / right.refreshRate.denominator));
                            }

                            return false;
                        }

                        return false;
                    });
                }

                return displayModeList;
            }

            void setFullScreenState(bool fullScreen)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(dxgiSwapChain);

                renderTargetViewCache.set(SwapChain, CComPtr<ID3D11RenderTargetView>(nullptr));
                d3dDeviceContext->ClearState();

                HRESULT resultValue = dxgiSwapChain->SetFullscreenState(fullScreen, nullptr);
                if (FAILED(resultValue))
                {
                    if (fullScreen)
                    {
                        throw Render::OperationFailed("Unablet to set fullscreen state");
                    }
                    else
                    {
                        throw Render::OperationFailed("Unablet to set windowed state");
                    }
                }

                updateSwapChain();
            }

            void setDisplayMode(const Render::DisplayMode &displayMode)
            {
                GEK_REQUIRE(dxgiSwapChain);

                renderTargetViewCache.set(SwapChain, CComPtr<ID3D11RenderTargetView>(nullptr));
                d3dDeviceContext->ClearState();

                DXGI_MODE_DESC description;
                description.Width = displayMode.width;
                description.Height = displayMode.height;
                description.RefreshRate.Numerator = displayMode.refreshRate.numerator;
                description.RefreshRate.Denominator = displayMode.refreshRate.denominator;
                description.Format = DirectX::TextureFormatList[static_cast<uint8_t>(displayMode.format)];
                description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                HRESULT resultValue = dxgiSwapChain->ResizeTarget(&description);
                if (FAILED(resultValue))
                {
                    throw Render::OperationFailed("Unable to set display mode");
                }

                updateSwapChain();
            }

            void handleResize(void)
            {
                GEK_REQUIRE(dxgiSwapChain);

                renderTargetViewCache.set(SwapChain, CComPtr<ID3D11RenderTargetView>(nullptr));
                d3dDeviceContext->ClearState();

                HRESULT resultValue = dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
                if (FAILED(resultValue))
                {
                    throw Render::OperationFailed("Unable to resize swap chain buffers to window size");
                }

                updateSwapChain();
            }

            void deletePipelineState(Render::PipelineStateHandle pipelineState)
            {
                pipelineStateCache.remove(pipelineState);
            }

            void deleteSamplerState(Render::SamplerStateHandle samplerState)
            {
                samplerStateCache.remove(samplerState);
            }

            void deleteResource(Render::ResourceHandle resource)
            {
                resourceCache.remove(resource);
                bufferCache.remove(resource);
                shaderResourceViewCache.remove(resource);
                unorderedAccessViewCache.remove(resource);
                renderTargetViewCache.remove(resource);
                depthStencilViewCache.remove(resource);
                textureDescriptionMap.erase(resource);
                bufferDescriptionMap.erase(resource);
            }

            void deleteQueue(Render::QueueHandle queue)
            {
                queueCache.remove(queue);
            }

            Render::PipelineStateHandle createPipelineState(const Render::PipelineStateInformation &pipelineStateInformation, std::string const &name)
            {
                return pipelineStateCache.insert(pipelineStateInformation.getHash(), [this, pipelineStateInformation, name = name](Render::PipelineStateHandle) -> CComPtr<PipelineState>
                {
                    auto shaderHeader = getShaderHeader(pipelineStateInformation);
                    auto compiledVertexShader = compileShader("", "vs_5_0", pipelineStateInformation.vertexShaderEntryFunction, pipelineStateInformation.vertexShader, shaderHeader);
                    auto compiledPixelShader = compileShader("", "ps_5_0", pipelineStateInformation.pixelShaderEntryFunction, pipelineStateInformation.pixelShader, shaderHeader);
                    auto vertexDeclaration = getVertexDeclaration(pipelineStateInformation.vertexDeclaration);

                    CComPtr<PipelineState> pipelineState(new PipelineState());
                    pipelineState->rasterizerState = createRasterizerState(pipelineStateInformation.rasterizerStateInformation);
                    pipelineState->depthStencilState = createDepthState(pipelineStateInformation.depthStateInformation);
                    pipelineState->blendState = createBlendState(pipelineStateInformation.blendStateInformation);
                    pipelineState->primitiveTopology = DirectX::TopologList[static_cast<uint8_t>(pipelineStateInformation.primitiveType)];
                    pipelineState->sampleMask = pipelineStateInformation.sampleMask;

                    d3dDevice->CreateInputLayout(vertexDeclaration.data(), vertexDeclaration.size(), compiledVertexShader.data(), compiledVertexShader.size(), &pipelineState->vertexDeclaration);
                    if (!pipelineState->vertexDeclaration)
                    {
                        throw Render::CreateObjectFailed("Unable to create pipeline vertex declaration");
                    }

                    d3dDevice->CreateVertexShader(compiledVertexShader.data(), compiledVertexShader.size(), nullptr, &pipelineState->vertexShader);
                    if (!pipelineState->vertexShader)
                    {
                        throw Render::CreateObjectFailed("Unable to create pipeline vertex shader");
                    }

                    d3dDevice->CreatePixelShader(compiledPixelShader.data(), compiledPixelShader.size(), nullptr, &pipelineState->pixelShader);
                    if (!pipelineState->pixelShader)
                    {
                        throw Render::CreateObjectFailed("Unable to create pipeline pixel shader");
                    }

                    setDebugName(pipelineState->rasterizerState, name, "RasterizerState");
                    setDebugName(pipelineState->depthStencilState, name, "DepthStencilState");
                    setDebugName(pipelineState->blendState, name, "BlendState");
                    setDebugName(pipelineState->vertexDeclaration, name, "InputLayout");
                    setDebugName(pipelineState->vertexShader, name, "VertexShader");
                    setDebugName(pipelineState->pixelShader, name, "PixelShader");
                    return pipelineState;
                });
            }

            Render::SamplerStateHandle createSamplerState(const Render::SamplerStateInformation &samplerStateInformation, std::string const &name)
            {
                GEK_REQUIRE(d3dDevice);

                return samplerStateCache.insert(samplerStateInformation.getHash(), [this, samplerStateInformation, name = std::string(name)](Render::SamplerStateHandle) -> CComPtr<ID3D11SamplerState>
                {
                    D3D11_SAMPLER_DESC samplerDescription;
                    samplerDescription.AddressU = DirectX::AddressModeList[static_cast<uint8_t>(samplerStateInformation.addressModeU)];
                    samplerDescription.AddressV = DirectX::AddressModeList[static_cast<uint8_t>(samplerStateInformation.addressModeV)];
                    samplerDescription.AddressW = DirectX::AddressModeList[static_cast<uint8_t>(samplerStateInformation.addressModeW)];
                    samplerDescription.MipLODBias = samplerStateInformation.mipLevelBias;
                    samplerDescription.MaxAnisotropy = samplerStateInformation.maximumAnisotropy;
                    samplerDescription.ComparisonFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(samplerStateInformation.comparisonFunction)];
                    samplerDescription.BorderColor[0] = samplerStateInformation.borderColor.r;
                    samplerDescription.BorderColor[1] = samplerStateInformation.borderColor.g;
                    samplerDescription.BorderColor[2] = samplerStateInformation.borderColor.b;
                    samplerDescription.BorderColor[3] = samplerStateInformation.borderColor.a;
                    samplerDescription.MinLOD = samplerStateInformation.minimumMipLevel;
                    samplerDescription.MaxLOD = samplerStateInformation.maximumMipLevel;
                    samplerDescription.Filter = DirectX::FilterList[static_cast<uint8_t>(samplerStateInformation.filterMode)];

                    CComPtr<ID3D11SamplerState> samplerState;
                    d3dDevice->CreateSamplerState(&samplerDescription, &samplerState);
                    setDebugName(samplerState, name);
                    return samplerState;
                });
            }

            bool mapResource(Render::ResourceHandle resource, void *&data, Render::Map mapping)
            {
                GEK_REQUIRE(d3dDeviceContext);

                D3D11_MAP d3dMapping = DirectX::MapList[static_cast<uint8_t>(mapping)];

                D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                mappedSubResource.pData = nullptr;
                mappedSubResource.RowPitch = 0;
                mappedSubResource.DepthPitch = 0;

                if (SUCCEEDED(d3dDeviceContext->Map(resourceCache.get(resource), 0, d3dMapping, 0, &mappedSubResource)))
                {
                    data = mappedSubResource.pData;
                    return true;
                }

                return false;
            }

            void unmapResource(Render::ResourceHandle resource)
            {
                GEK_REQUIRE(d3dDeviceContext);

                d3dDeviceContext->Unmap(resourceCache.get(resource), 0);
            }

            void updateResource(Render::ResourceHandle resource, const void *data)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(data);

                d3dDeviceContext->UpdateSubresource(resourceCache.get(resource), 0, nullptr, data, 0, 0);
            }

            void copyResource(Render::ResourceHandle destination, Render::ResourceHandle source)
            {
                GEK_REQUIRE(d3dDeviceContext);

                /*
                auto destinationTexture = dynamic_cast<BaseResourceHandle >(destination);
                auto sourceTexture = dynamic_cast<BaseResourceHandle >(source);
                if (destinationTexture && sourceTexture)
                {
                    if (destinationTexture->description.width != sourceTexture->description.width ||
                        destinationTexture->description.height != sourceTexture->description.height ||
                        destinationTexture->description.depth != sourceTexture->description.depth)
                    {
                        return;
                    }
                }

                if (destinationTexture->description.mipMapCount > 0 || sourceTexture->description.mipMapCount > 0)
                {
                    d3dDeviceContext->CopySubresourceRegion(getObject<Resource>(destination), 0, 0, 0, 0, getObject<Resource>(source), 0, nullptr);
                }
                else
                {
                    d3dDeviceContext->CopyResource(getObject<Resource>(destination), getObject<Resource>(source));
                }
                */
            }

            Render::ResourceHandle createBuffer(const Render::BufferDescription &description, const void *data, std::string const &name)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(description.count > 0);

                auto dataHash = reinterpret_cast<size_t>(data);
                auto hash = CombineHashes(description.getHash(), dataHash);
                return resourceCache.insert(hash, [this, description, data, name = std::string(name)](Render::ResourceHandle handle) -> CComPtr<ID3D11Resource>
                {
                    uint32_t stride = description.stride;
                    if (description.format != Render::Format::Unknown)
                    {
                        if (description.stride > 0)
                        {
                            throw Render::InvalidParameter("Buffer requires only a format or an element stride");
                        }

                        stride = DirectX::FormatStrideList[static_cast<uint8_t>(description.format)];
                    }
                    else if (description.stride == 0)
                    {
                        throw Render::InvalidParameter("Buffer requires either a format or an element stride");
                    }

                    D3D11_BUFFER_DESC bufferDescription;
                    bufferDescription.ByteWidth = (stride * description.count);
                    switch (description.type)
                    {
                    case Render::BufferDescription::Type::Structured:
                        bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                        bufferDescription.StructureByteStride = stride;
                        bufferDescription.BindFlags = 0;
                        break;

                    case Render::BufferDescription::Type::IndirectArguments:
                        bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
                        bufferDescription.StructureByteStride = stride;
                        bufferDescription.BindFlags = 0;
                        break;

                    default:
                        bufferDescription.MiscFlags = 0;
                        bufferDescription.StructureByteStride = 0;
                        switch (description.type)
                        {
                        case Render::BufferDescription::Type::Vertex:
                            bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                            break;

                        case Render::BufferDescription::Type::Index:
                            bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                            break;

                        case Render::BufferDescription::Type::Constant:
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
                    else if (description.flags & Render::BufferDescription::Flags::Staging)
                    {
                        bufferDescription.Usage = D3D11_USAGE_STAGING;
                        bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                    }
                    else if (description.flags & Render::BufferDescription::Flags::Mappable)
                    {
                        bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
                        bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    }
                    else
                    {
                        bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                        bufferDescription.CPUAccessFlags = 0;
                    }

                    if (description.flags & Render::BufferDescription::Flags::Resource)
                    {
                        bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                    }

                    if (description.flags & Render::BufferDescription::Flags::UnorderedAccess)
                    {
                        bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                    }

                    CComPtr<ID3D11Buffer> d3dBuffer;
                    if (data == nullptr)
                    {
                        HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer);
                        if (FAILED(resultValue) || !d3dBuffer)
                        {
                            throw Render::CreateObjectFailed("Unable to dynamic buffer");
                        }
                    }
                    else
                    {
                        D3D11_SUBRESOURCE_DATA resourceData;
                        resourceData.pSysMem = data;
                        resourceData.SysMemPitch = 0;
                        resourceData.SysMemSlicePitch = 0;
                        HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, &resourceData, &d3dBuffer);
                        if (FAILED(resultValue) || !d3dBuffer)
                        {
                            throw Render::CreateObjectFailed("Unable to create static buffer");
                        }
                    }

                    setDebugName(d3dBuffer, name);
                    bufferCache.set(handle, d3dBuffer);
                    if (description.flags & Render::BufferDescription::Flags::Resource)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                        viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(description.format)];
                        viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = description.count;

                        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                        HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                        if (FAILED(resultValue) || !d3dShaderResourceView)
                        {
                            throw Render::CreateObjectFailed("Unable to create buffer shader resource view");
                        }

                        setDebugName(d3dShaderResourceView, name, "ShaderResourceView");
                        shaderResourceViewCache.set(handle, d3dShaderResourceView);
                    }

                    if (description.flags & Render::BufferDescription::Flags::UnorderedAccess)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                        viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(description.format)];
                        viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                        viewDescription.Buffer.FirstElement = 0;
                        viewDescription.Buffer.NumElements = description.count;
                        viewDescription.Buffer.Flags = (description.flags & Render::BufferDescription::Flags::Counter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0);

                        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                        HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                        if (FAILED(resultValue) || !d3dUnorderedAccessView)
                        {
                            throw Render::CreateObjectFailed("Unable to create buffer unordered access view");
                        }

                        setDebugName(d3dUnorderedAccessView, name, "UnorderedAccessView");
                        unorderedAccessViewCache.set(handle, d3dUnorderedAccessView);
                    }

                    bufferDescriptionMap.insert(std::make_pair(handle, description));
                    return CComQIPtr<ID3D11Resource>(d3dBuffer);
                });
            }

            Render::ResourceHandle createTexture(const Render::TextureDescription &description, const void *data, std::string const &name)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(description.format != Render::Format::Unknown);
                GEK_REQUIRE(description.width != 0);
                GEK_REQUIRE(description.height != 0);
                GEK_REQUIRE(description.depth != 0);

                auto dataHash = reinterpret_cast<size_t>(data);
                auto hash = CombineHashes(description.getHash(), dataHash);
                return resourceCache.insert(hash, [this, description, data, name = std::string(name)](Render::ResourceHandle handle)->CComPtr<ID3D11Resource>
                {
                    uint32_t bindFlags = 0;
                    if (description.flags & Render::TextureDescription::Flags::RenderTarget)
                    {
                        if (description.flags & Render::TextureDescription::Flags::DepthTarget)
                        {
                            throw Render::InvalidParameter("Cannot create render target when depth target flag also specified");
                        }

                        bindFlags |= D3D11_BIND_RENDER_TARGET;
                    }

                    if (description.flags & Render::TextureDescription::Flags::DepthTarget)
                    {
                        if (description.depth > 1)
                        {
                            throw Render::InvalidParameter("Depth target must have depth of one");
                        }

                        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
                    }

                    if (description.flags & Render::TextureDescription::Flags::Resource)
                    {
                        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
                    }

                    if (description.flags & Render::TextureDescription::Flags::UnorderedAccess)
                    {
                        bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                    }

                    D3D11_SUBRESOURCE_DATA resourceData;
                    resourceData.pSysMem = data;
                    resourceData.SysMemPitch = (DirectX::FormatStrideList[static_cast<uint8_t>(description.format)] * description.width);
                    resourceData.SysMemSlicePitch = (description.depth == 1 ? 0 : (resourceData.SysMemPitch * description.height));

                    CComQIPtr<ID3D11Resource> d3dResource;
                    if (description.depth == 1)
                    {
                        D3D11_TEXTURE2D_DESC textureDescription;
                        textureDescription.Width = description.width;
                        textureDescription.Height = description.height;
                        textureDescription.MipLevels = description.mipMapCount;
                        textureDescription.Format = DirectX::TextureFormatList[static_cast<uint8_t>(description.format)];
                        textureDescription.ArraySize = 1;
                        textureDescription.SampleDesc.Count = description.sampleCount;
                        textureDescription.SampleDesc.Quality = description.sampleQuality;
                        textureDescription.BindFlags = bindFlags;
                        textureDescription.CPUAccessFlags = 0;
                        textureDescription.MiscFlags = (description.mipMapCount == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);
                        if (data == nullptr)
                        {
                            textureDescription.Usage = D3D11_USAGE_DEFAULT;
                        }
                        else
                        {
                            textureDescription.Usage = D3D11_USAGE_IMMUTABLE;
                        }

                        CComPtr<ID3D11Texture2D> texture2D;
                        HRESULT resultValue = d3dDevice->CreateTexture2D(&textureDescription, (data ? &resourceData : nullptr), &texture2D);
                        if (FAILED(resultValue) || !texture2D)
                        {
                            throw Render::CreateObjectFailed("Unable to create 2D texture");
                        }

                        d3dResource = texture2D;
                    }
                    else
                    {
                        D3D11_TEXTURE3D_DESC textureDescription;
                        textureDescription.Width = description.width;
                        textureDescription.Height = description.height;
                        textureDescription.Depth = description.depth;
                        textureDescription.MipLevels = description.mipMapCount;
                        textureDescription.Format = DirectX::TextureFormatList[static_cast<uint8_t>(description.format)];
                        textureDescription.BindFlags = bindFlags;
                        textureDescription.CPUAccessFlags = 0;
                        textureDescription.MiscFlags = (description.mipMapCount == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);
                        if (data == nullptr)
                        {
                            textureDescription.Usage = D3D11_USAGE_DEFAULT;
                        }
                        else
                        {
                            textureDescription.Usage = D3D11_USAGE_IMMUTABLE;
                        }

                        CComPtr<ID3D11Texture3D> texture3D;
                        HRESULT resultValue = d3dDevice->CreateTexture3D(&textureDescription, (data ? &resourceData : nullptr), &texture3D);
                        if (FAILED(resultValue) || !texture3D)
                        {
                            throw Render::CreateObjectFailed("Unable to create 3D texture");
                        }

                        d3dResource = texture3D;
                    }

                    if (!d3dResource)
                    {
                        throw Render::CreateObjectFailed("Unable to get texture resource");
                    }

                    setDebugName(d3dResource, name);
                    if (description.flags & Render::TextureDescription::Flags::Resource)
                    {
                        D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                        viewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(description.format)];
                        if (description.depth == 1)
                        {
                            viewDescription.ViewDimension = (description.sampleCount > 1 ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D);
                            viewDescription.Texture2D.MostDetailedMip = 0;
                            viewDescription.Texture2D.MipLevels = -1;
                        }
                        else
                        {
                            viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                            viewDescription.Texture3D.MostDetailedMip = 0;
                            viewDescription.Texture3D.MipLevels = -1;
                        }

                        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                        HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dResource, &viewDescription, &d3dShaderResourceView);
                        if (FAILED(resultValue) || !d3dShaderResourceView)
                        {
                            throw Render::CreateObjectFailed("Unable to create texture shader resource view");
                        }

                        setDebugName(d3dShaderResourceView, name, "ShaderResourceView");
                        shaderResourceViewCache.set(handle, d3dShaderResourceView);
                    }

                    if (description.flags & Render::TextureDescription::Flags::UnorderedAccess)
                    {
                        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                        viewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(description.format)];
                        if (description.depth == 1)
                        {
                            viewDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                            viewDescription.Texture2D.MipSlice = 0;
                        }
                        else
                        {
                            viewDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                            viewDescription.Texture3D.MipSlice = 0;
                            viewDescription.Texture3D.FirstWSlice = 0;
                            viewDescription.Texture3D.WSize = description.depth;
                        }

                        CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                        HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView);
                        if (FAILED(resultValue) || !d3dUnorderedAccessView)
                        {
                            throw Render::CreateObjectFailed("Unable to create texture unordered access view");
                        }

                        setDebugName(d3dUnorderedAccessView, name, "UnorderedAccessView");
                        unorderedAccessViewCache.set(handle, d3dUnorderedAccessView);
                    }

                    if (description.flags & Render::TextureDescription::Flags::RenderTarget)
                    {
                        D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                        renderViewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(description.format)];
                        if (description.depth == 1)
                        {
                            renderViewDescription.ViewDimension = (description.sampleCount > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D);
                            renderViewDescription.Texture2D.MipSlice = 0;
                        }
                        else
                        {
                            renderViewDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                            renderViewDescription.Texture3D.MipSlice = 0;
                            renderViewDescription.Texture3D.FirstWSlice = 0;
                            renderViewDescription.Texture3D.WSize = description.depth;
                        }

                        CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                        HRESULT resultValue = d3dDevice->CreateRenderTargetView(d3dResource, &renderViewDescription, &d3dRenderTargetView);
                        if (FAILED(resultValue) || !d3dRenderTargetView)
                        {
                            throw Render::CreateObjectFailed("Unable to create render target view");
                        }

                        setDebugName(d3dRenderTargetView, name, "RenderTargetView");
                        renderTargetViewCache.set(handle, d3dRenderTargetView);
                    }
                    else if (description.flags & Render::TextureDescription::Flags::DepthTarget)
                    {
                        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                        depthStencilDescription.Format = DirectX::DepthFormatList[static_cast<uint8_t>(description.format)];
                        depthStencilDescription.ViewDimension = (description.sampleCount > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
                        depthStencilDescription.Flags = 0;
                        depthStencilDescription.Texture2D.MipSlice = 0;

                        CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
                        HRESULT resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &d3dDepthStencilView);
                        if (FAILED(resultValue) || !d3dDepthStencilView)
                        {
                            throw Render::CreateObjectFailed("Unable to create depth stencil view");
                        }

                        setDebugName(d3dDepthStencilView, name, "DepthStencilView");
                        depthStencilViewCache.set(handle, d3dDepthStencilView);
                    }

                    textureDescriptionMap.insert(std::make_pair(handle, description));
                    return d3dResource;
                });
            }

            Render::ResourceHandle loadTexture(FileSystem::Path const &filePath, uint32_t flags, std::string const &name)
            {
                GEK_REQUIRE(d3dDevice);

                auto hash = GetHash(0xFFFFFFFF, filePath.u8string(), flags);
                return resourceCache.insert(hash, [this, filePath, flags, name = std::string(name)](Render::ResourceHandle handle)->CComPtr<ID3D11Resource>
                {
					static const std::vector<uint8_t> EmptyBuffer;
					std::vector<uint8_t> buffer(FileSystem::Load(filePath, EmptyBuffer));

                    std::string extension(String::GetLower(filePath.getExtension()));
                    std::function<HRESULT(const std::vector<uint8_t> &, ::DirectX::ScratchImage &)> load;
                    if (extension == ".dds")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromDDSMemory(buffer.data(), buffer.size(), 0, nullptr, image); };
                    }
                    else if (extension == ".tga")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
                    }
                    else if (extension == ".png")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_PNG, nullptr, image); };
                    }
                    else if (extension == ".bmp")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_BMP, nullptr, image); };
                    }
                    else if (extension == ".jpg" || extension == ".jpeg")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_JPEG, nullptr, image); };
                    }
                    else if (extension == ".tif" || extension == ".tiff")
                    {
                        load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_TIFF, nullptr, image); };
                    }

                    if (!load)
                    {
                        throw Render::InvalidFileType("Unknown texture extension encountered");
                    }

                    ::DirectX::ScratchImage image;
                    HRESULT resultValue = load(buffer, image);
                    if (FAILED(resultValue))
                    {
                        throw Render::LoadFileFailed("Unable to load texture from file");
                    }

                    CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                    resultValue = ::DirectX::CreateShaderResourceViewEx(d3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, (flags & Render::TextureLoadFlags::sRGB), &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        throw Render::CreateObjectFailed("Unable to create texture shader resource view");
                    }

                    CComPtr<ID3D11Resource> d3dResource;
                    d3dShaderResourceView->GetResource(&d3dResource);
                    if (!d3dResource)
                    {
                        throw Render::CreateObjectFailed("Unable to get texture resource");
                    }

					setDebugName(d3dResource, (name.empty() ? filePath.u8string() : name));
                    setDebugName(d3dShaderResourceView, (name.empty() ? filePath.u8string() : name), "ShaderResourceView");
                    shaderResourceViewCache.set(handle, d3dShaderResourceView);

                    Render::TextureDescription description;
                    description.width = image.GetMetadata().width;
                    description.height = image.GetMetadata().height;
                    description.depth = image.GetMetadata().depth;
                    description.format = DirectX::getFormat(image.GetMetadata().format);
                    description.mipMapCount = image.GetMetadata().mipLevels;
                    textureDescriptionMap.insert(std::make_pair(handle, description));
                    return d3dResource;
                });
            }

            Render::BufferDescription const * const getBufferDescription(Render::ResourceHandle resource) const
            {
                auto descriptionSearch = bufferDescriptionMap.find(resource);
                if (descriptionSearch != std::end(bufferDescriptionMap))
                {
                    return &descriptionSearch->second;
                }

                return nullptr;
            }

            Render::TextureDescription const * const getTextureDescription(Render::ResourceHandle resource) const
            {
                auto descriptionSearch = textureDescriptionMap.find(resource);
                if (descriptionSearch != std::end(textureDescriptionMap))
                {
                    return &descriptionSearch->second;
                }

                return nullptr;
            }

            Render::Device::QueuePtr createQueue(uint32_t flags, std::string const &name)
            {
                CComPtr<ID3D11DeviceContext> d3dDeviceContext;
                HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeviceContext);
                if (!d3dDeviceContext)
                {
                    throw Render::CreateObjectFailed("Unable to create bundle render queue");
                }

                setDebugName(d3dDeviceContext, name);
                return std::make_unique<Queue>(this, d3dDeviceContext);
            }

            Render::QueueHandle compileQueue(Render::Device::Queue *baseQueue, std::string const &name)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(baseQueue);

                Queue *queue = dynamic_cast<Queue *>(baseQueue);
                if (!queue)
                {
                    throw Render::CreateObjectFailed("Unable to get internal render queue");
                }

                CComPtr<ID3D11CommandList> d3dCommandList;
                HRESULT resultValue = queue->d3dDeviceContext->FinishCommandList(false, &d3dCommandList);
                if (FAILED(resultValue) || !d3dCommandList)
                {
                    throw Render::CreateObjectFailed("Unable to create render list");
                }

                setDebugName(d3dCommandList, name);
                return queueCache.insert(d3dCommandList);
            }

            void runQueue(Render::Device::Queue *baseQueue)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(baseQueue);

                Queue *queue = dynamic_cast<Queue *>(baseQueue);
                if (!queue)
                {
                    throw Render::CreateObjectFailed("Unable to get internal render queue");
                }

                CComPtr<ID3D11CommandList> commandList;
                HRESULT resultValue = queue->d3dDeviceContext->FinishCommandList(false, &commandList);
                if (FAILED(resultValue) || !commandList)
                {
                    throw Render::CreateObjectFailed("Unable to create render list");
                }

                d3dDeviceContext->ExecuteCommandList(commandList, false);
            }

            void runQueue(Render::QueueHandle queue)
            {
                auto commandList = queueCache.get(queue);
                if (commandList)
                {
                    d3dDeviceContext->ExecuteCommandList(commandList, false);
                }
            }

            void present(bool waitForVerticalSync)
            {
                GEK_REQUIRE(dxgiSwapChain);

                dxgiSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Direct3D11
}; // namespace Gek
