#pragma warning(disable : 4005)

#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Hash.hpp"
#include "GEK/Render/Device.hpp"
#include "GEK/Render/Window.hpp"
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <wincodec.h>
#include <atlbase.h>
#include <dxgi1_3.h>
#include <algorithm>
#include <comdef.h>
#include <d3d11.h>
#include <memory>
#include <ppl.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
    namespace Direct3D11
    {
        // All these lists must match, since the same GEK Format can be used for either textures or buffers
        // The size list must also match
		static constexpr DXGI_FORMAT TextureFormatList[] =
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

		static constexpr DXGI_FORMAT DepthFormatList[] =
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

		static constexpr DXGI_FORMAT ViewFormatList[] =
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

		static constexpr DXGI_FORMAT BufferFormatList[] =
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

		static constexpr uint32_t FormatStrideList[] =
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

		static constexpr D3D11_DEPTH_WRITE_MASK DepthWriteMaskList[] =
        {
            D3D11_DEPTH_WRITE_MASK_ZERO,
            D3D11_DEPTH_WRITE_MASK_ALL,
        };

		static constexpr D3D11_TEXTURE_ADDRESS_MODE AddressModeList[] =
        {
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
            D3D11_TEXTURE_ADDRESS_BORDER,
        };

        static constexpr D3D11_COMPARISON_FUNC ComparisonFunctionList[] =
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

		static constexpr D3D11_STENCIL_OP StencilOperationList[] =
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

		static constexpr D3D11_BLEND BlendSourceList[] =
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

		static constexpr D3D11_BLEND_OP BlendOperationList[] =
        {
            D3D11_BLEND_OP_ADD,
            D3D11_BLEND_OP_SUBTRACT,
            D3D11_BLEND_OP_REV_SUBTRACT,
            D3D11_BLEND_OP_MIN,
            D3D11_BLEND_OP_MAX,
        };

		static constexpr D3D11_FILL_MODE FillModeList[] =
        {
            D3D11_FILL_WIREFRAME,
            D3D11_FILL_SOLID,
        };

		static constexpr D3D11_CULL_MODE CullModeList[] =
        {
            D3D11_CULL_NONE,
            D3D11_CULL_FRONT,
            D3D11_CULL_BACK,
        };

		static constexpr D3D11_FILTER FilterList[] =
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

		static constexpr D3D11_PRIMITIVE_TOPOLOGY TopologList[] =
        {
            D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

		static constexpr D3D11_MAP MapList[] =
        {
            D3D11_MAP_READ,
            D3D11_MAP_WRITE,
            D3D11_MAP_READ_WRITE,
            D3D11_MAP_WRITE_DISCARD,
            D3D11_MAP_WRITE_NO_OVERWRITE,
        };

		static constexpr std::string_view VertexSemanticList[] =
        {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMA",
            "NORMAL",
            "COLOR",
        };

		static constexpr std::string_view PixelSemanticList[] =
        {
            "SV_POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMA",
            "NORMAL",
            "COLOR",
        };

        static_assert(ARRAYSIZE(VertexSemanticList) == static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count), "New element semantic added without adding to all VertexSemanticList.");
        static_assert(ARRAYSIZE(PixelSemanticList) == static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count), "New element semantic added without adding to all PixelSemanticList.");

        Render::Format GetFormat(DXGI_FORMAT format)
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

        template <typename CLASS>
        void SetDebugName(CComPtr<CLASS> &object, std::string const &name, std::string const &member = String::Empty)
        {
			auto finalName(name + (member.empty() ? "::" : String::Empty) + (member.empty() ? member : String::Empty));
            object->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(finalName.size()), finalName.data());
        }

        template <typename D3DTYPE, typename D3DSOURCE>
        void AtomicAddRef(D3DTYPE** d3dObject, CComPtr<D3DSOURCE>& d3dSource)
        {
            if (d3dSource)
            {
                InterlockedExchangePointer(reinterpret_cast<void**>(d3dObject), dynamic_cast<D3DTYPE*>(d3dSource.p));
                (*d3dObject)->AddRef();
            }
        }

        template <typename D3DTYPE>
        void AtomicRelease(D3DTYPE** d3dObject)
        {
            if (*d3dObject)
            {
                reinterpret_cast<D3DTYPE*>(InterlockedExchangePointer((void**)d3dObject, nullptr))->Release();
            }
        }

        GEK_CONTEXT_USER(Device, Window *, Render::Device::Description)
            , public Render::Debug::Device
        {
            class PipelineState
                : public Render::PipelineState
            {
            public:
                Render::PipelineState::Description description;
                ID3D11RasterizerState* d3dRasterizerState = nullptr;
                ID3D11DepthStencilState *d3dDepthStencilState = nullptr;
                ID3D11BlendState *d3dBlendState = nullptr;
                ID3D11InputLayout *d3dVertexLayout = nullptr;
                ID3D11VertexShader *d3dVertexShader = nullptr;
                ID3D11PixelShader *d3dPixelShader = nullptr;
                D3D11_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
                uint32_t d3dSampleMask = 0x0;

            public:
                PipelineState(const Render::PipelineState::Description& description,
                    CComPtr<ID3D11RasterizerState>& d3dRasterizerState,
                    CComPtr<ID3D11DepthStencilState>& d3dDepthStencilState,
                    CComPtr<ID3D11BlendState>& d3dBlendState,
                    CComPtr<ID3D11InputLayout>& d3dVertexLayout,
                    CComPtr<ID3D11VertexShader>& d3dVertexShader,
                    CComPtr<ID3D11PixelShader>& d3dPixelShader,
                    D3D11_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology,
                    uint32_t d3dSampleMask)
                    : description(description)
                    , d3dPrimitiveTopology(d3dPrimitiveTopology)
                    , d3dSampleMask(d3dSampleMask)
                {
                    AtomicAddRef(&this->d3dRasterizerState, d3dRasterizerState);
                    AtomicAddRef(&this->d3dDepthStencilState, d3dDepthStencilState);
                    AtomicAddRef(&this->d3dBlendState, d3dBlendState);
                    AtomicAddRef(&this->d3dVertexLayout, d3dVertexLayout);
                    AtomicAddRef(&this->d3dVertexShader, d3dVertexShader);
                    AtomicAddRef(&this->d3dPixelShader, d3dPixelShader);
                }

                virtual ~PipelineState(void)
                {
                    AtomicRelease(&d3dPixelShader);
                    AtomicRelease(&d3dVertexShader);
                    AtomicRelease(&d3dVertexLayout);
                    AtomicRelease(&d3dBlendState);
                    AtomicRelease(&d3dDepthStencilState);
                    AtomicRelease(&d3dRasterizerState);
                }

                std::string_view getName(void) const
                {
                    return description.name;
                }

                Render::PipelineState::Description const& getDescription(void) const
                {
                    return description;
                }
            };

            class SamplerState
                : public Render::SamplerState
            {
            public:
                Render::SamplerState::Description description;
                ID3D11SamplerState* d3dSamplerState = nullptr;

            public:
                SamplerState(const Render::SamplerState::Description& description, CComPtr<ID3D11SamplerState>& d3dSamplerState)
                    : description(description)
                {
                    AtomicAddRef(&this->d3dSamplerState, d3dSamplerState);
                }

                virtual ~SamplerState(void)
                {
                    AtomicRelease(&d3dSamplerState);
                }

                std::string_view getName(void) const
                {
                    return description.name;
                }

                Render::SamplerState::Description const& getDescription(void) const
                {
                    return description;
                }
            };

            class ViewResource
            {
            public:
                ID3D11ShaderResourceView* d3dShaderResourceView = nullptr;

            public:
                ViewResource(CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView)
                {
                    AtomicAddRef(&this->d3dShaderResourceView, d3dShaderResourceView);
                }

                virtual ~ViewResource(void)
                {
                    AtomicRelease(&d3dShaderResourceView);
                }
            };

            class UnorderedResource
            {
            public:
                ID3D11UnorderedAccessView* d3dUnorderedAccessView = nullptr;

            public:
                UnorderedResource(CComPtr<ID3D11UnorderedAccessView>& d3dUnorderedAccessView)
                {
                    AtomicAddRef(&this->d3dUnorderedAccessView, d3dUnorderedAccessView);
                }

                virtual ~UnorderedResource(void)
                {
                    AtomicRelease(&d3dUnorderedAccessView);
                }
            };

            class TargetResource
            {
            public:
                ID3D11RenderTargetView* d3dRenderTargetView = nullptr;

            public:
                TargetResource(CComPtr<ID3D11RenderTargetView>& d3dRenderTargetView)
                {
                    AtomicAddRef(&this->d3dRenderTargetView, d3dRenderTargetView);
                }

                virtual ~TargetResource(void)
                {
                    AtomicRelease(&d3dRenderTargetView);
                }
            };

            class Buffer
                : public Render::Buffer
                , public ViewResource
                , public UnorderedResource
            {
            public:
                Render::Buffer::Description description;
                ID3D11Buffer* d3dBuffer = nullptr;

            public:
                Buffer(const Render::Buffer::Description& description,
                    CComPtr<ID3D11Buffer>& d3dBuffer,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView,
                    CComPtr<ID3D11UnorderedAccessView>& d3dUnorderedAccessView)
                    : ViewResource(d3dShaderResourceView)
                    , UnorderedResource(d3dUnorderedAccessView)
                    , description(description)
                {
                    AtomicAddRef(&this->d3dBuffer, d3dBuffer);
                }

                virtual ~Buffer(void)
                {
                    AtomicRelease(&d3dBuffer);
                }

                std::string_view getName(void) const
                {
                    return description.name;
                }

                Render::Buffer::Description const& getDescription(void) const
                {
                    return description;
                }
            };

            class Texture
                : public Render::Texture
                , public ViewResource
            {
            public:
                Render::Texture::Description description;
                ID3D11Resource* d3dBaseResource = nullptr;

            public:
                Texture(const Render::Texture::Description& description,
                    CComPtr<ID3D11Resource>& d3dBaseResource,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView)
                    : ViewResource(d3dShaderResourceView)
                    , description(description)
                {
                    AtomicAddRef(&this->d3dBaseResource, d3dBaseResource);
                }

                virtual ~Texture(void)
                {
                    AtomicRelease(&d3dBaseResource);
                }

                std::string_view getName(void) const
                {
                    return description.name;
                }

                Render::Texture::Description const& getDescription(void) const
                {
                    return description;
                }
            };

            class UnorderedTexture
                : public Texture
                , public UnorderedResource
            {
            public:
                UnorderedTexture(const Render::Texture::Description& description,
                    CComPtr<ID3D11Resource>& d3dBaseResource,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView,
                    CComPtr<ID3D11UnorderedAccessView>& d3dUnorderedAccessView)
                    : Texture(description, d3dBaseResource, d3dShaderResourceView)
                    , UnorderedResource(d3dUnorderedAccessView)
                {
                }

                virtual ~UnorderedTexture(void)
                {
                }
            };

            class TargetTexture
                : public UnorderedTexture
                , public TargetResource
            {
            public:
                TargetTexture(const Render::Texture::Description& description,
                    CComPtr<ID3D11Resource>& d3dBaseResource,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView,
                    CComPtr<ID3D11UnorderedAccessView>& d3dUnorderedAccessView,
                    CComPtr<ID3D11RenderTargetView>& d3dRenderTargetView)
                    : UnorderedTexture(description, d3dBaseResource, d3dShaderResourceView, d3dUnorderedAccessView)
                    , TargetResource(d3dRenderTargetView)
                {
                }

                virtual ~TargetTexture(void)
                {
                }
            };

            class BackBuffer
                : public Texture
                , public TargetResource
            {
            public:
                BackBuffer(const Render::Texture::Description& description,
                    CComPtr<ID3D11Resource>& d3dBaseResource,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView,
                    CComPtr<ID3D11RenderTargetView>& d3dRenderTargetView)
                    : Texture(description, d3dBaseResource, d3dShaderResourceView)
                    , TargetResource(d3dRenderTargetView)
                {
                }

                virtual ~BackBuffer(void)
                {
                }
            };

            class DepthTexture
                : public UnorderedTexture
            {
            public:
                ID3D11DepthStencilView* d3dDepthStencilView = nullptr;

            public:
                DepthTexture(const Render::Texture::Description& description,
                    CComPtr<ID3D11Resource>& d3dBaseResource,
                    CComPtr<ID3D11ShaderResourceView>& d3dShaderResourceView,
                    CComPtr<ID3D11UnorderedAccessView>& d3dUnorderedAccessView,
                    CComPtr<ID3D11DepthStencilView> &d3dDepthStencilView)
                    : UnorderedTexture(description, d3dBaseResource, d3dShaderResourceView, d3dUnorderedAccessView)
                {
                    AtomicAddRef(&this->d3dDepthStencilView, d3dDepthStencilView);
                }

                virtual ~DepthTexture(void)
                {
                    AtomicRelease(&d3dDepthStencilView);
                }
            };

            class CommandList
                : public Render::CommandList
            {
            public:
                Device *device = nullptr;
                ID3D11DeviceContext* d3dDeferredContext = nullptr;
                ID3D11CommandList* d3dCommandList = nullptr;

            public:
                CommandList(Device *device, CComPtr<ID3D11DeviceContext> &d3dDeferredContext)
                    : device(device)
                {
                    assert(d3dDeferredContext);
                    AtomicAddRef(&this->d3dDeferredContext, d3dDeferredContext);
                }

                virtual ~CommandList(void)
                {
                    AtomicRelease(&d3dCommandList);
                    AtomicRelease(&d3dDeferredContext);
                }

                std::string_view getName(void) const
                {
                    return "command_list";
                }

                // Render::Device::Queue
                void finish(void)
                {
                    assert(d3dDeferredContext);

                    CComPtr<ID3D11CommandList> d3dCommandList;
                    HRESULT resultValue = d3dDeferredContext->FinishCommandList(false, &d3dCommandList);
                    if (FAILED(resultValue) || !d3dCommandList)
                    {
                        std::cerr << "Unable to create render list";
                    }

                    AtomicAddRef(&this->d3dCommandList, d3dCommandList);
                }

                void generateMipMaps(Render::Resource *texture)
                {
                    assert(d3dDeferredContext);

                    auto internalTexture = dynamic_cast<Texture*>(texture);
                    if (internalTexture)
                    {
                        d3dDeferredContext->GenerateMips(internalTexture->d3dShaderResourceView);
                    }
                }

                void resolveSamples(Render::Resource* destination, Render::Resource* source)
                {
                    assert(d3dDeferredContext);

                    auto internalDestination = dynamic_cast<Texture*>(destination);
                    auto internalSource = dynamic_cast<Texture*>(source);
                    if (internalDestination && internalSource)
                    {
                        //d3dDeferredContext->ResolveSubresource(device->resourceCache.get(destination), 0, device->resourceCache.get(source), 0, DXGI_FORMAT_UNKNOWN);
                    }
                }

                void clearUnorderedAccess(Render::Resource* object, Math::Float4 const &value)
                {
                    assert(d3dDeferredContext);

                    auto internalObject = dynamic_cast<UnorderedResource*>(object);
                    if (internalObject)
                    {
                        d3dDeferredContext->ClearUnorderedAccessViewFloat(internalObject->d3dUnorderedAccessView, value.data);
                    }
                }

                void clearUnorderedAccess(Render::Resource* object, Math::UInt4 const &value)
                {
                    assert(d3dDeferredContext);

                    auto internalObject = dynamic_cast<UnorderedResource*>(object);
                    if (internalObject)
                    {
                        d3dDeferredContext->ClearUnorderedAccessViewUint(internalObject->d3dUnorderedAccessView, value.data);
                    }
                }

                void clearRenderTarget(Render::Texture* renderTarget, Math::Float4 const &clearColor)
                {
                    assert(d3dDeferredContext);

                    auto internalTarget = dynamic_cast<TargetResource*>(renderTarget);
                    if (internalTarget)
                    {
                        d3dDeferredContext->ClearRenderTargetView(internalTarget->d3dRenderTargetView, clearColor.data);
                    }
                }

                void clearDepthStencilTarget(Render::Texture* depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    assert(d3dDeferredContext);

                    auto internalBuffer = dynamic_cast<DepthTexture*>(depthBuffer);
                    if (internalBuffer)
                    {
                        d3dDeferredContext->ClearDepthStencilView(internalBuffer->d3dDepthStencilView,
                            ((flags & Render::ClearFlags::Depth ? D3D11_CLEAR_DEPTH : 0) |
                            (flags & Render::ClearFlags::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                            clearDepth, clearStencil);
                    }
                }

                void setViewportList(const std::vector<Render::ViewPort> &viewPortList)
                {
                    assert(d3dDeferredContext);

                    d3dDeferredContext->RSSetViewports(viewPortList.size(), (D3D11_VIEWPORT *)viewPortList.data());
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                    assert(d3dDeferredContext);

                    d3dDeferredContext->RSSetScissorRects(rectangleList.size(), (D3D11_RECT *)rectangleList.data());
                }

                void bindPipelineState(Render::PipelineState* pipelineState)
                {
                    assert(d3dDeferredContext);

                    auto internalPipelineState = dynamic_cast<PipelineState*>(pipelineState);
                    if (internalPipelineState)
                    {
                        d3dDeferredContext->RSSetState(internalPipelineState->d3dRasterizerState);
                        d3dDeferredContext->OMSetDepthStencilState(internalPipelineState->d3dDepthStencilState, 0x0);
                        d3dDeferredContext->OMSetBlendState(internalPipelineState->d3dBlendState, Math::Float4::Zero.data, internalPipelineState->d3dSampleMask);
                        d3dDeferredContext->IASetInputLayout(internalPipelineState->d3dVertexLayout);
                        d3dDeferredContext->VSSetShader(internalPipelineState->d3dVertexShader, nullptr, 0);
                        d3dDeferredContext->PSSetShader(internalPipelineState->d3dPixelShader, nullptr, 0);
                        d3dDeferredContext->IASetPrimitiveTopology(internalPipelineState->d3dPrimitiveTopology);
                    }
                }

                void bindSamplerStateList(const std::vector<Render::SamplerState*> & samplerStateList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11SamplerState*> d3dSamplerStateList;
                    for (auto& samplerState : samplerStateList)
                    {
                        auto internalSamplerState = dynamic_cast<SamplerState*>(samplerState);
                        if (internalSamplerState)
                        {
                            d3dSamplerStateList.push_back(internalSamplerState->d3dSamplerState);
                        }
                    }

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeferredContext->VSSetSamplers(firstStage, d3dSamplerStateList.size(), d3dSamplerStateList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeferredContext->PSSetSamplers(firstStage, d3dSamplerStateList.size(), d3dSamplerStateList.data());
                    }
                }

                void bindConstantBufferList(const std::vector<Render::Buffer*> &constantBufferList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11Buffer*> d3dconstantBufferList;
                    for (auto& constantBuffer : constantBufferList)
                    {
                        auto internalBuffer = dynamic_cast<Buffer*>(constantBuffer);
                        if (internalBuffer)
                        {
                            d3dconstantBufferList.push_back(internalBuffer->d3dBuffer);
                        }
                    }

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeferredContext->VSSetConstantBuffers(firstStage, d3dconstantBufferList.size(), d3dconstantBufferList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeferredContext->PSSetConstantBuffers(firstStage, d3dconstantBufferList.size(), d3dconstantBufferList.data());
                    }
                }

                void bindResourceList(const std::vector<Render::Resource*> &resourceList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11ShaderResourceView*> d3dResourceList;
                    for (auto& resource : resourceList)
                    {
                        auto internalResource = dynamic_cast<ViewResource*>(resource);
                        if (internalResource)
                        {
                            d3dResourceList.push_back(internalResource->d3dShaderResourceView);
                        }
                    }

                    if (pipelineFlags & Render::Pipeline::Vertex)
                    {
                        d3dDeferredContext->VSSetShaderResources(firstStage, d3dResourceList.size(), d3dResourceList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        d3dDeferredContext->PSSetShaderResources(firstStage, d3dResourceList.size(), d3dResourceList.data());
                    }
                }

                void bindUnorderedAccessList(const std::vector<Render::Resource*> &unorderedResourceList, uint32_t firstStage, uint32_t *countList)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11UnorderedAccessView*> d3dResourceList;
                    for (auto& unorderedResource : unorderedResourceList)
                    {
                        auto internalResource = dynamic_cast<UnorderedResource*>(unorderedResource);
                        if (internalResource)
                        {
                            d3dResourceList.push_back(internalResource->d3dUnorderedAccessView);
                        }
                    }

                    d3dDeferredContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, d3dResourceList.size(), d3dResourceList.data(), countList);
                }

                void bindIndexBuffer(Render::Resource* indexBuffer, uint32_t offset)
                {
                    assert(d3dDeferredContext);

                    auto internalBuffer = dynamic_cast<Buffer*>(indexBuffer);
                    if (internalBuffer)
                    {
                        DXGI_FORMAT format = Direct3D11::BufferFormatList[static_cast<uint8_t>(internalBuffer->description.format)];
                        d3dDeferredContext->IASetIndexBuffer(internalBuffer->d3dBuffer, format, offset);
                    }
                }

                void bindVertexBufferList(const std::vector<Render::Resource*> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11Buffer*> d3dBufferList;
                    std::vector<uint32_t> d3dVertexStrideList;
                    std::vector<uint32_t> d3dVertexOffsetList;
                    for (auto& vertexBuffer : vertexBufferList)
                    {
                        auto internalBuffer = dynamic_cast<Buffer*>(vertexBuffer);
                        if (internalBuffer)
                        {
                            d3dBufferList.push_back(internalBuffer->d3dBuffer);
                            d3dVertexStrideList.push_back(internalBuffer->description.stride);
                            d3dVertexOffsetList.push_back(0);
                        }
                    }

                    d3dDeferredContext->IASetVertexBuffers(firstSlot, d3dBufferList.size(), d3dBufferList.data(), d3dVertexStrideList.data(), offsetList ? offsetList : d3dVertexOffsetList.data());
                }

                void bindRenderTargetList(const std::vector<Render::Texture*> &renderTargetList, Render::Texture* depthBuffer)
                {
                    assert(d3dDeferredContext);

                    std::vector<ID3D11RenderTargetView*> d3dTargetList;
                    for (auto& renderTarget : renderTargetList)
                    {
                        auto internalTarget = dynamic_cast<TargetResource*>(renderTarget);
                        if (internalTarget)
                        {
                            d3dTargetList.push_back(internalTarget->d3dRenderTargetView);
                        }
                    }

                    auto internalDepth = dynamic_cast<DepthTexture*>(depthBuffer);
                    if (internalDepth)
                    {
                        d3dDeferredContext->OMSetRenderTargets(d3dTargetList.size(), d3dTargetList.data(), internalDepth->d3dDepthStencilView);
                    }
                    else
                    {
                        d3dDeferredContext->OMSetRenderTargets(d3dTargetList.size(), d3dTargetList.data(), nullptr);
                    }
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    assert(d3dDeferredContext);

                    d3dDeferredContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    assert(d3dDeferredContext);

                    d3dDeferredContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                    assert(d3dDeferredContext);

                    d3dDeferredContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }

                void drawInstancedPrimitive(Render::Resource* bufferArguments)
                {
                    assert(d3dDeferredContext);
                    
                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        d3dDeferredContext->DrawInstancedIndirect(internalBuffer->d3dBuffer, 0);
                    }
                }

                void drawInstancedIndexedPrimitive(Render::Resource* bufferArguments)
                {
                    assert(d3dDeferredContext);
                    
                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        d3dDeferredContext->DrawIndexedInstancedIndirect(internalBuffer->d3dBuffer, 0);
                    }
                }

                void dispatch(Render::Resource* bufferArguments)
                {
                    assert(d3dDeferredContext);
                    
                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        d3dDeferredContext->DispatchIndirect(internalBuffer->d3dBuffer, 0);
                    }
                }
            };

            class Queue
                : public Render::Queue
            {
            public:
                Device* device = nullptr;
                ID3D11DeviceContext* d3dDeviceContext = nullptr;

            public:
                Queue(Device* device, ID3D11DeviceContext* d3dDeviceContext)
                    : device(device)
                    , d3dDeviceContext(d3dDeviceContext)
                {
                }

                std::string_view getName(void) const
                {
                    return "queue";
                }

                void executeCommandList(Render::CommandList *commandList)
                {
                    assert(d3dDeviceContext);
                    assert(commandList);

                    auto d3dCommandList = dynamic_cast<CommandList *>(commandList);
                    if (d3dCommandList)
                    {
                        d3dDeviceContext->ExecuteCommandList(d3dCommandList->d3dCommandList, false);
                    }
                }
            };

        public:
            Window *window = nullptr;
            bool isChildWindow = false;

            CComPtr<ID3D11Device> d3dDevice;
            CComPtr<ID3D11DeviceContext> d3dDefaultContext;
            CComPtr<IDXGISwapChain1> dxgiSwapChain;
            Render::TexturePtr backBuffer;

        public:
            Device(Gek::Context *context, Window *window, Render::Device::Description deviceDescription)
                : ContextRegistration(context)
                , window(window)
                , isChildWindow(GetParent((HWND)window->getBaseWindow()) != nullptr)
            {
                assert(window);

                UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
                flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

                D3D_FEATURE_LEVEL featureLevelList[] =
                {
                    D3D_FEATURE_LEVEL_11_0,
                };

                D3D_FEATURE_LEVEL featureLevel;
                HRESULT resultValue = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, 1, D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dDefaultContext);
                if (featureLevel != featureLevelList[0])
                {
                    throw std::runtime_error("Direct3D 11.0 feature level required");
                }

                if (FAILED(resultValue) || !d3dDevice || !d3dDefaultContext)
                {
                    throw std::runtime_error("Unable to create rendering device and context");
                }

                CComPtr<IDXGIFactory2> dxgiFactory;
                resultValue = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
                if (FAILED(resultValue) || !dxgiFactory)
                {
                    throw std::runtime_error("Unable to get graphics factory");
                }

                DXGI_SWAP_CHAIN_DESC1 swapChainDescription;
                swapChainDescription.Width = 0;
                swapChainDescription.Height = 0;
                swapChainDescription.Format = Direct3D11::TextureFormatList[static_cast<uint8_t>(deviceDescription.displayFormat)];
                swapChainDescription.Stereo = false;
                swapChainDescription.SampleDesc.Count = deviceDescription.sampleCount;
                swapChainDescription.SampleDesc.Quality = deviceDescription.sampleQuality;
                swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
                swapChainDescription.BufferCount = 2;
                swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
                swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                swapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
                resultValue = dxgiFactory->CreateSwapChainForHwnd(d3dDevice, (HWND)window->getBaseWindow(), &swapChainDescription, nullptr, nullptr, &dxgiSwapChain);
                if (FAILED(resultValue) || !dxgiSwapChain)
                {
                    throw std::runtime_error("Unable to create swap chain for window");
                }

                dxgiFactory->MakeWindowAssociation((HWND)window->getBaseWindow(), 0);

#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                CComQIPtr<ID3D11InfoQueue> d3dInfoQueue(d3dDebug);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                //d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif
            }

            ~Device(void)
            {
                setFullScreenState(false);

                dxgiSwapChain.Release();
                d3dDefaultContext.Release();
#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
                d3dDevice.Release();
            }

            std::mutex backBufferMutex;
            Render::Texture* getBackBuffer(void)
            {
                if (!backBuffer)
                {
                    std::unique_lock<std::mutex> lock(backBufferMutex);
                    if (!backBuffer)
                    {
                        CComPtr<ID3D11Texture2D> d3dRenderTarget;
                        HRESULT resultValue = dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                        if (FAILED(resultValue) || !d3dRenderTarget)
                        {
                            throw std::runtime_error("Unable to get swap chain primary buffer");
                        }

                        CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                        resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                        if (FAILED(resultValue) || !d3dRenderTargetView)
                        {
                            throw std::runtime_error("Unable to create render target view for back buffer");
                        }

                        CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                        resultValue = d3dDevice->CreateShaderResourceView(d3dRenderTarget, nullptr, &d3dShaderResourceView);
                        if (FAILED(resultValue) || !d3dShaderResourceView)
                        {
                            throw std::runtime_error("Unable to create shader resource view for back buffer");
                        }

                        D3D11_TEXTURE2D_DESC textureDescription;
                        d3dRenderTarget->GetDesc(&textureDescription);

                        Render::Texture::Description description;
                        description.width = textureDescription.Width;
                        description.height = textureDescription.Height;
                        description.format = Direct3D11::GetFormat(textureDescription.Format);
                        CComQIPtr<ID3D11Resource> d3dResource(d3dRenderTarget);
                        backBuffer = std::make_unique<BackBuffer>(description, d3dResource, d3dShaderResourceView, d3dRenderTargetView);
                    }
                }

                return backBuffer.get();
            }

            CComPtr<ID3D11RasterizerState> createRasterizerState(const Render::PipelineState::RasterizerState::Description &rasterizerStateDescription)
            {
                assert(d3dDevice);

                D3D11_RASTERIZER_DESC rasterizerDescription;
                rasterizerDescription.FrontCounterClockwise = rasterizerStateDescription.frontCounterClockwise;
                rasterizerDescription.DepthBias = rasterizerStateDescription.depthBias;
                rasterizerDescription.DepthBiasClamp = rasterizerStateDescription.depthBiasClamp;
                rasterizerDescription.SlopeScaledDepthBias = rasterizerStateDescription.slopeScaledDepthBias;
                rasterizerDescription.DepthClipEnable = rasterizerStateDescription.depthClipEnable;
                rasterizerDescription.ScissorEnable = rasterizerStateDescription.scissorEnable;
                rasterizerDescription.MultisampleEnable = rasterizerStateDescription.multisampleEnable;
                rasterizerDescription.AntialiasedLineEnable = rasterizerStateDescription.antialiasedLineEnable;
                rasterizerDescription.FillMode = Direct3D11::FillModeList[static_cast<uint8_t>(rasterizerStateDescription.fillMode)];
                rasterizerDescription.CullMode = Direct3D11::CullModeList[static_cast<uint8_t>(rasterizerStateDescription.cullMode)];

                CComPtr<ID3D11RasterizerState> rasterizerState;
                d3dDevice->CreateRasterizerState(&rasterizerDescription, &rasterizerState);
                if (!rasterizerState)
                {
                    std::cerr << "Unable to create rasterizer state";
                    return nullptr;
                }

                return rasterizerState;
            }

            CComPtr<ID3D11DepthStencilState> createDepthState(const Render::PipelineState::DepthState::Description &depthStateDescription)
            {
                assert(d3dDevice);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = depthStateDescription.enable;
                depthStencilDescription.DepthFunc = Direct3D11::ComparisonFunctionList[static_cast<uint8_t>(depthStateDescription.comparisonFunction)];
                depthStencilDescription.StencilEnable = depthStateDescription.stencilEnable;
                depthStencilDescription.StencilReadMask = depthStateDescription.stencilReadMask;
                depthStencilDescription.StencilWriteMask = depthStateDescription.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilFrontState.failOperation)];
                depthStencilDescription.FrontFace.StencilDepthFailOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilFrontState.depthFailOperation)];
                depthStencilDescription.FrontFace.StencilPassOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilFrontState.passOperation)];
                depthStencilDescription.FrontFace.StencilFunc = Direct3D11::ComparisonFunctionList[static_cast<uint8_t>(depthStateDescription.stencilFrontState.comparisonFunction)];
                depthStencilDescription.BackFace.StencilFailOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilBackState.failOperation)];
                depthStencilDescription.BackFace.StencilDepthFailOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilBackState.depthFailOperation)];
                depthStencilDescription.BackFace.StencilPassOp = Direct3D11::StencilOperationList[static_cast<uint8_t>(depthStateDescription.stencilBackState.passOperation)];
                depthStencilDescription.BackFace.StencilFunc = Direct3D11::ComparisonFunctionList[static_cast<uint8_t>(depthStateDescription.stencilBackState.comparisonFunction)];
                depthStencilDescription.DepthWriteMask = Direct3D11::DepthWriteMaskList[static_cast<uint8_t>(depthStateDescription.writeMask)];

                CComPtr<ID3D11DepthStencilState> depthState;
                d3dDevice->CreateDepthStencilState(&depthStencilDescription, &depthState);
                if (!depthState)
                {
                    std::cerr << "Unable to create depth stencil state";
                    return nullptr;
                }

                return depthState;
            }

            CComPtr<ID3D11BlendState> createBlendState(const Render::PipelineState::BlendState::Description &blendStateDescription)
            {
                assert(d3dDevice);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendStateDescription.alphaToCoverage;
                blendDescription.IndependentBlendEnable = !blendStateDescription.unifiedBlendState;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    blendDescription.RenderTarget[renderTarget].BlendEnable = blendStateDescription.targetStateList[renderTarget].enable;
                    blendDescription.RenderTarget[renderTarget].SrcBlend = Direct3D11::BlendSourceList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].colorSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlend = Direct3D11::BlendSourceList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].colorDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOp = Direct3D11::BlendOperationList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].colorOperation)];
                    blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = Direct3D11::BlendSourceList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].alphaSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlendAlpha = Direct3D11::BlendSourceList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].alphaDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOpAlpha = Direct3D11::BlendOperationList[static_cast<uint8_t>(blendStateDescription.targetStateList[renderTarget].alphaOperation)];
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                    if (blendStateDescription.targetStateList[renderTarget].writeMask & Render::PipelineState::BlendState::Mask::R)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (blendStateDescription.targetStateList[renderTarget].writeMask & Render::PipelineState::BlendState::Mask::G)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (blendStateDescription.targetStateList[renderTarget].writeMask & Render::PipelineState::BlendState::Mask::B)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (blendStateDescription.targetStateList[renderTarget].writeMask & Render::PipelineState::BlendState::Mask::A)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                CComPtr<ID3D11BlendState> blendState;
                d3dDevice->CreateBlendState(&blendDescription, &blendState);
                if (!blendState)
                {
                    std::cerr << "Unable to create blend state";
                    return nullptr;
                }

                return blendState;
            }

            std::vector<D3D11_INPUT_ELEMENT_DESC> getVertexDeclaration(const std::vector<Render::PipelineState::VertexDeclaration> &vertexDeclaration)
            {
                uint32_t semanticIndexList[static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count)] = { 0 };
                std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescriptionList;
                for (auto const &vertexElement : vertexDeclaration)
                {
                    D3D11_INPUT_ELEMENT_DESC elementDescription;
                    elementDescription.Format = Direct3D11::BufferFormatList[static_cast<uint8_t>(vertexElement.format)];
                    elementDescription.AlignedByteOffset = (vertexElement.alignedByteOffset == Render::PipelineState::VertexDeclaration::AppendAligned ? D3D11_APPEND_ALIGNED_ELEMENT : vertexElement.alignedByteOffset);
                    elementDescription.SemanticName = Direct3D11::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)].data();
                    elementDescription.SemanticIndex = semanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++;
                    elementDescription.InputSlot = vertexElement.sourceIndex;
                    switch (vertexElement.source)
                    {
                    case Render::PipelineState::VertexDeclaration::Source::Instance:
                        elementDescription.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        elementDescription.InstanceDataStepRate = 1;
                        break;

                    case Render::PipelineState::VertexDeclaration::Source::Vertex:
                    default:
                        elementDescription.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        elementDescription.InstanceDataStepRate = 0;
                        break;
                    };

                    inputElementDescriptionList.push_back(elementDescription);
                }

                return inputElementDescriptionList;
            }

            std::string getShaderHeader(const Render::PipelineState::Description &pipelineStateDescription)
            {
                std::vector<std::string> vertexData;
                uint32_t vertexSemanticIndexList[static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count)] = { 0 };
                for (auto const &vertexElement : pipelineStateDescription.vertexDeclaration)
                {
                    std::string semantic = Direct3D11::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)].data();
                    std::string format = Direct3D11::getFormatSemantic(vertexElement.format);
                    vertexData.push_back(std::format("    {} {} : {}{};", format, vertexElement.name, semantic, vertexSemanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++));
                }

                std::vector<std::string> pixelData;
                uint32_t pixelSemanticIndexList[static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count)] = { 0 };
                for (auto const &pixelElement : pipelineStateDescription.pixelDeclaration)
                {
                    std::string semantic = Direct3D11::PixelSemanticList[static_cast<uint8_t>(pixelElement.semantic)].data();
                    std::string format = Direct3D11::getFormatSemantic(pixelElement.format);
                    pixelData.push_back(std::format("    {} {} : {}{};", format, pixelElement.name, semantic, pixelSemanticIndexList[static_cast<uint8_t>(pixelElement.semantic)]++));
                }

                uint32_t renderTargetIndex = 0;
                std::vector<std::string> renderTargets;
                for (auto const &renderTarget : pipelineStateDescription.renderTargetList)
                {
                    std::string format = Direct3D11::getFormatSemantic(renderTarget.format);
                    renderTargets.push_back(std::format("    {} {} : SV_TARGET{};", format, renderTarget.name, renderTargets.size()));
                }

                static constexpr std::string_view shaderHeaderTemplate =
R"(#define DeclareConstantBuffer(NAME, INDEX) cbuffer NAME : register(b##INDEX)
#define DeclareSamplerState(NAME, INDEX) SamplerState NAME : register(s##INDEX)
#define DeclareTexture1D(NAME, TYPE, INDEX) Texture1D<TYPE> NAME : register(t##INDEX)
#define DeclareTexture2D(NAME, TYPE, INDEX) Texture2D<TYPE> NAME : register(t##INDEX)
#define DeclareTexture3D(NAME, TYPE, INDEX) Texture3D<TYPE> NAME : register(t##INDEX)
#define DeclareTextureCube(NAME, TYPE, INDEX) TextureCube<TYPE> NAME : register(t##INDEX)
#define SampleTexture(TEXTURE, SAMPLER, COORD) TEXTURE.Sample(SAMPLER, COORD)

struct Vertex
{{
    {}
}};

struct Pixel
{{
    {}
}};

struct Output
{{
    {}
}};)";

                auto vertexString = String::Join(vertexData, "\r\n");
                auto pixelString = String::Join(pixelData, "\r\n");
                auto renderString = String::Join(renderTargets, "\r\n");
                return std::vformat(shaderHeaderTemplate, std::make_format_args(vertexString, pixelString, renderString));
            }

            std::vector<uint8_t> compileShader(std::string const &name, std::string const &type, std::string const &entryFunction, std::string const &shader, const std::string &header)
            {
                assert(d3dDevice);

                uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
                flags |= D3DCOMPILE_DEBUG;
                flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
                flags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
                flags |= D3DCOMPILE_SKIP_VALIDATION;
                flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
                auto fullShader(std::format("{}{}", header, shader));

                CComPtr<ID3DBlob> d3dShaderBlob;
                CComPtr<ID3DBlob> d3dCompilerErrors;
                HRESULT resultValue = D3DCompile(fullShader.data(), (fullShader.size() + 1), name.data(), nullptr, nullptr, entryFunction.data(), type.data(), flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
                if (FAILED(resultValue) || !d3dShaderBlob)
                {
                    _com_error error(resultValue);
                    std::string_view compilerError = (char const *)d3dCompilerErrors->GetBufferPointer();
                    std::cerr << "D3DCompile Failed (" << error.ErrorMessage() << ") " << compilerError;
                    static const std::vector<uint8_t> EmptyBuffer;
                    return EmptyBuffer;
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
                    dxgiOutput->GetDisplayModeList(Direct3D11::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, nullptr);

                    std::vector<DXGI_MODE_DESC> dxgiDisplayModeList(modeCount);
                    dxgiOutput->GetDisplayModeList(Direct3D11::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, dxgiDisplayModeList.data());

                    for (auto const &dxgiDisplayMode : dxgiDisplayModeList)
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
                            displayMode.format = Direct3D11::GetFormat(dxgiDisplayMode.Format);
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
                assert(d3dDefaultContext);
                assert(dxgiSwapChain);

                backBuffer = nullptr;
                d3dDefaultContext->ClearState();

                HRESULT resultValue = dxgiSwapChain->SetFullscreenState(fullScreen, nullptr);
                if (FAILED(resultValue))
                {
                    if (fullScreen)
                    {
                        throw std::runtime_error("Unablet to set fullscreen state");
                    }
                    else
                    {
                        throw std::runtime_error("Unablet to set windowed state");
                    }
                }
            }

            void setDisplayMode(const Render::DisplayMode &displayMode)
            {
                assert(dxgiSwapChain);

                backBuffer = nullptr;
                d3dDefaultContext->ClearState();

                DXGI_MODE_DESC description;
                description.Width = displayMode.width;
                description.Height = displayMode.height;
                description.RefreshRate.Numerator = displayMode.refreshRate.numerator;
                description.RefreshRate.Denominator = displayMode.refreshRate.denominator;
                description.Format = Direct3D11::TextureFormatList[static_cast<uint8_t>(displayMode.format)];
                description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                HRESULT resultValue = dxgiSwapChain->ResizeTarget(&description);
                if (FAILED(resultValue))
                {
                    throw std::runtime_error("Unable to set display mode");
                }
            }

            void handleResize(void)
            {
                assert(dxgiSwapChain);

                backBuffer = nullptr;
                d3dDefaultContext->ClearState();

                HRESULT resultValue = dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
                if (FAILED(resultValue))
                {
                    throw std::runtime_error("Unable to resize swap chain buffers to window size");
                }
            }

            Render::PipelineStatePtr createPipelineState(const Render::PipelineState::Description &pipelineStateDescription)
            {
                auto shaderHeader = getShaderHeader(pipelineStateDescription);
                auto compiledVertexShader = compileShader("", "vs_5_0", pipelineStateDescription.vertexShaderEntryFunction, pipelineStateDescription.vertexShader, shaderHeader);
                auto compiledPixelShader = compileShader("", "ps_5_0", pipelineStateDescription.pixelShaderEntryFunction, pipelineStateDescription.pixelShader, shaderHeader);
                auto vertexDeclaration = getVertexDeclaration(pipelineStateDescription.vertexDeclaration);

                auto rasterizerState = createRasterizerState(pipelineStateDescription.rasterizerStateDescription);
                auto depthStencilState = createDepthState(pipelineStateDescription.depthStateDescription);
                auto blendState = createBlendState(pipelineStateDescription.blendStateDescription);
                auto primitiveTopology = Direct3D11::TopologList[static_cast<uint8_t>(pipelineStateDescription.primitiveType)];
                auto sampleMask = pipelineStateDescription.sampleMask;

                CComPtr<ID3D11InputLayout> vertexLayout;
                d3dDevice->CreateInputLayout(vertexDeclaration.data(), vertexDeclaration.size(), compiledVertexShader.data(), compiledVertexShader.size(), &vertexLayout);
                if (!vertexLayout)
                {
                    std::cerr << "Unable to create pipeline vertex declaration";
                    return nullptr;
                }

                CComPtr<ID3D11VertexShader> vertexShader;
                d3dDevice->CreateVertexShader(compiledVertexShader.data(), compiledVertexShader.size(), nullptr, &vertexShader);
                if (!vertexShader)
                {
                    std::cerr << "Unable to create pipeline vertex shader";
                    return nullptr;
                }

                CComPtr<ID3D11PixelShader> pixelShader;
                d3dDevice->CreatePixelShader(compiledPixelShader.data(), compiledPixelShader.size(), nullptr, &pixelShader);
                if (!pixelShader)
                {
                    std::cerr << "Unable to create pipeline pixel shader";
                    return nullptr;
                }

                SetDebugName(rasterizerState, pipelineStateDescription.name, "RasterizerState");
                SetDebugName(depthStencilState, pipelineStateDescription.name, "DepthStencilState");
                SetDebugName(blendState, pipelineStateDescription.name, "BlendState");
                SetDebugName(vertexLayout, pipelineStateDescription.name, "VertexLayout");
                SetDebugName(vertexShader, pipelineStateDescription.name, "VertexShader");
                SetDebugName(pixelShader, pipelineStateDescription.name, "PixelShader");
                return std::make_unique<PipelineState>(pipelineStateDescription,
                    rasterizerState,
                    depthStencilState,
                    blendState,
                    vertexLayout,
                    vertexShader,
                    pixelShader,
                    primitiveTopology,
                    sampleMask);
            }

            Render::SamplerStatePtr createSamplerState(const Render::SamplerState::Description &samplerStateDescription)
            {
                assert(d3dDevice);

                D3D11_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = Direct3D11::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeU)];
                samplerDescription.AddressV = Direct3D11::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeV)];
                samplerDescription.AddressW = Direct3D11::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeW)];
                samplerDescription.MipLODBias = samplerStateDescription.mipLevelBias;
                samplerDescription.MaxAnisotropy = samplerStateDescription.maximumAnisotropy;
                samplerDescription.ComparisonFunc = Direct3D11::ComparisonFunctionList[static_cast<uint8_t>(samplerStateDescription.comparisonFunction)];
                samplerDescription.BorderColor[0] = samplerStateDescription.borderColor.r;
                samplerDescription.BorderColor[1] = samplerStateDescription.borderColor.g;
                samplerDescription.BorderColor[2] = samplerStateDescription.borderColor.b;
                samplerDescription.BorderColor[3] = samplerStateDescription.borderColor.a;
                samplerDescription.MinLOD = samplerStateDescription.minimumMipLevel;
                samplerDescription.MaxLOD = samplerStateDescription.maximumMipLevel;
                samplerDescription.Filter = Direct3D11::FilterList[static_cast<uint8_t>(samplerStateDescription.filterMode)];

                CComPtr<ID3D11SamplerState> samplerState;
                d3dDevice->CreateSamplerState(&samplerDescription, &samplerState);
                SetDebugName(samplerState, samplerStateDescription.name);
                return std::make_unique<SamplerState>(samplerStateDescription, samplerState);
            }

            bool mapResource(Render::Resource* resource, void *&data, Render::Map mapping)
            {
                assert(d3dDefaultContext);

                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (internalBuffer)
                {
                    D3D11_MAP d3dMapping = Direct3D11::MapList[static_cast<uint8_t>(mapping)];

                    D3D11_MAPPED_SUBRESOURCE d3dMappedSubResource;
                    d3dMappedSubResource.pData = nullptr;
                    d3dMappedSubResource.RowPitch = 0;
                    d3dMappedSubResource.DepthPitch = 0;
                    if (SUCCEEDED(d3dDefaultContext->Map(internalBuffer->d3dBuffer, 0, d3dMapping, 0, &d3dMappedSubResource)))
                    {
                        data = d3dMappedSubResource.pData;
                        return true;
                    }
                }

                return false;
            }

            void unmapResource(Render::Resource* resource)
            {
                assert(d3dDefaultContext);

                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (internalBuffer)
                {
                    d3dDefaultContext->Unmap(internalBuffer->d3dBuffer, 0);
                }
            }

            void updateResource(Render::Resource* resource, const void *data)
            {
                assert(d3dDefaultContext);
                assert(data);

                auto internalBuffer = dynamic_cast<Buffer*>(resource);
                if (internalBuffer)
                {
                    d3dDefaultContext->UpdateSubresource(internalBuffer->d3dBuffer, 0, nullptr, data, 0, 0);
                }
            }

            void copyResource(Render::Resource* destination, Render::Resource* source)
            {
                assert(d3dDefaultContext);

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
                    d3dDefaultContext->CopySubresourceRegion(getObject<Resource>(destination), 0, 0, 0, 0, getObject<Resource>(source), 0, nullptr);
                }
                else
                {
                    d3dDefaultContext->CopyResource(getObject<Resource>(destination), getObject<Resource>(source));
                }
                */
            }

            Render::BufferPtr createBuffer(const Render::Buffer::Description &description, const void *data)
            {
                assert(d3dDevice);
                assert(description.count > 0);

                uint32_t stride = description.stride;
                if (description.format != Render::Format::Unknown)
                {
                    if (description.stride > 0)
                    {
                        std::cerr << "Buffer requires only a format or an element stride";
                        return nullptr;
                    }

                    stride = Direct3D11::FormatStrideList[static_cast<uint8_t>(description.format)];
                }
                else if (description.stride == 0)
                {
                    std::cerr << "Buffer requires either a format or an element stride";
                    return nullptr;
                }

                D3D11_BUFFER_DESC bufferDescription;
                bufferDescription.ByteWidth = (stride * description.count);
                switch (description.type)
                {
                case Render::Buffer::Type::Structured:
                    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                    bufferDescription.StructureByteStride = stride;
                    bufferDescription.BindFlags = 0;
                    break;

                case Render::Buffer::Type::IndirectArguments:
                    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
                    bufferDescription.StructureByteStride = stride;
                    bufferDescription.BindFlags = 0;
                    break;

                default:
                    bufferDescription.MiscFlags = 0;
                    bufferDescription.StructureByteStride = 0;
                    switch (description.type)
                    {
                    case Render::Buffer::Type::Vertex:
                        bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                        break;

                    case Render::Buffer::Type::Index:
                        bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                        break;

                    case Render::Buffer::Type::Constant:
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
                else if (description.flags & Render::Buffer::Flags::Staging)
                {
                    bufferDescription.Usage = D3D11_USAGE_STAGING;
                    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                }
                else if (description.flags & Render::Buffer::Flags::Mappable)
                {
                    bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
                    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                }
                else
                {
                    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                    bufferDescription.CPUAccessFlags = 0;
                }

                if (description.flags & Render::Buffer::Flags::Resource)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (description.flags & Render::Buffer::Flags::UnorderedAccess)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                CComPtr<ID3D11Buffer> d3dBuffer;
                if (data == nullptr)
                {
                    HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer);
                    if (FAILED(resultValue) || !d3dBuffer)
                    {
                        std::cerr << "Unable to dynamic buffer";
                        return nullptr;
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
                        std::cerr << "Unable to create static buffer";
                        return nullptr;
                    }
                }

                SetDebugName(d3dBuffer, description.name);
                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Render::Buffer::Flags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = Direct3D11::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;

                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        std::cerr << "Unable to create buffer shader resource view";
                        return nullptr;
                    }

                    SetDebugName(d3dShaderResourceView, description.name, "ShaderResourceView");
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Render::Buffer::Flags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = Direct3D11::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;
                    viewDescription.Buffer.Flags = (description.flags & Render::Buffer::Flags::Counter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0);

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        std::cerr << "Unable to create buffer unordered access view";
                        return nullptr;
                    }

                    SetDebugName(d3dUnorderedAccessView, description.name, "UnorderedAccessView");
                }

                return std::make_unique<Buffer>(description, d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView);
            }

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data)
            {
                assert(d3dDevice);
                assert(description.format != Render::Format::Unknown);
                assert(description.width != 0);
                assert(description.height != 0);
                assert(description.depth != 0);

                uint32_t bindFlags = 0;
                if (description.flags & Render::Texture::Flags::RenderTarget)
                {
                    if (description.flags & Render::Texture::Flags::DepthTarget)
                    {
                        std::cerr << "Cannot create render target when depth target flag also specified";
                        return nullptr;
                    }

                    bindFlags |= D3D11_BIND_RENDER_TARGET;
                }

                if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    if (description.depth > 1)
                    {
                        std::cerr << "Depth target must have depth of one";
                        return nullptr;
                    }

                    bindFlags |= D3D11_BIND_DEPTH_STENCIL;
                }

                if (description.flags & Render::Texture::Flags::Resource)
                {
                    bindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (description.flags & Render::Texture::Flags::UnorderedAccess)
                {
                    bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                D3D11_SUBRESOURCE_DATA resourceData;
                resourceData.pSysMem = data;
                resourceData.SysMemPitch = (Direct3D11::FormatStrideList[static_cast<uint8_t>(description.format)] * description.width);
                resourceData.SysMemSlicePitch = (description.depth == 1 ? 0 : (resourceData.SysMemPitch * description.height));

                CComQIPtr<ID3D11Resource> d3dResource;
                if (description.depth == 1)
                {
                    D3D11_TEXTURE2D_DESC textureDescription;
                    textureDescription.Width = description.width;
                    textureDescription.Height = description.height;
                    textureDescription.MipLevels = description.mipMapCount;
                    textureDescription.Format = Direct3D11::TextureFormatList[static_cast<uint8_t>(description.format)];
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
                        std::cerr << "Unable to create 2D texture";
                        return nullptr;
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
                    textureDescription.Format = Direct3D11::TextureFormatList[static_cast<uint8_t>(description.format)];
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
                        std::cerr << "Unable to create 3D texture";
                        return nullptr;
                    }

                    d3dResource = texture3D;
                }

                if (!d3dResource)
                {
                    std::cerr << "Unable to get texture resource";
                    return nullptr;
                }

                SetDebugName(d3dResource, description.name);
                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Render::Texture::Flags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = Direct3D11::ViewFormatList[static_cast<uint8_t>(description.format)];
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

                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dResource, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        std::cerr << "Unable to create texture shader resource view";
                        return nullptr;
                    }

                    SetDebugName(d3dShaderResourceView, description.name, "ShaderResourceView");
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Render::Texture::Flags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = Direct3D11::ViewFormatList[static_cast<uint8_t>(description.format)];
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

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        std::cerr << "Unable to create texture unordered access view";
                        return nullptr;
                    }

                    SetDebugName(d3dUnorderedAccessView, description.name, "UnorderedAccessView");
                }

                if (description.flags & Render::Texture::Flags::RenderTarget)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                    renderViewDescription.Format = Direct3D11::ViewFormatList[static_cast<uint8_t>(description.format)];
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
                        std::cerr << "Unable to create render target view";
                        return nullptr;
                    }

                    SetDebugName(d3dRenderTargetView, description.name, "RenderTargetView");
                    return std::make_unique<TargetTexture>(description, d3dResource, d3dShaderResourceView, d3dUnorderedAccessView, d3dRenderTargetView);
                }
                else if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = Direct3D11::DepthFormatList[static_cast<uint8_t>(description.format)];
                    depthStencilDescription.ViewDimension = (description.sampleCount > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
                    HRESULT resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &d3dDepthStencilView);
                    if (FAILED(resultValue) || !d3dDepthStencilView)
                    {
                        std::cerr << "Unable to create depth stencil view";
                        return nullptr;
                    }

                    SetDebugName(d3dDepthStencilView, description.name, "DepthStencilView");
                    return std::make_unique<DepthTexture>(description, d3dResource, d3dShaderResourceView, d3dUnorderedAccessView, d3dDepthStencilView);
                }

                return std::make_unique<UnorderedTexture>(description, d3dResource, d3dShaderResourceView, d3dUnorderedAccessView);
            }

            Render::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                assert(d3dDevice);

				static const std::vector<uint8_t> EmptyBuffer;
				std::vector<uint8_t> buffer(FileSystem::Load(filePath));

                std::string extension(String::GetLower(filePath.getExtension()));
                std::function<HRESULT(const std::vector<uint8_t> &, ::DirectX::ScratchImage &)> load;
                if (extension == ".dds")
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromDDSMemory(buffer.data(), buffer.size(), ::DirectX::DDS_FLAGS_NONE, nullptr, image); };
                }
                else if (extension == ".tga")
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
                }
                else if (extension == ".png" || extension == ".bmp" ||
                            extension == ".jpg" || extension == ".jpeg" ||
                            extension == ".tif" || extension == ".tiff")
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_FLAGS_NONE, nullptr, image); };
                }

                if (!load)
                {
                    std::cerr << "Unknown texture extension encountered";
                    return nullptr;
                }

                ::DirectX::ScratchImage image;
                HRESULT resultValue = load(buffer, image);
                if (FAILED(resultValue))
                {
                    std::cerr << "Unable to load texture from file";
                    return nullptr;
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                auto createFlags = (flags & Render::TextureLoadFlags::sRGB ? ::DirectX::CREATETEX_FORCE_SRGB : ::DirectX::CREATETEX_DEFAULT);
                resultValue = ::DirectX::CreateShaderResourceViewEx(d3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, createFlags, &d3dShaderResourceView);
                if (FAILED(resultValue) || !d3dShaderResourceView)
                {
                    std::cerr << "Unable to create texture shader resource view";
                    return nullptr;
                }

                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                if (!d3dResource)
                {
                    std::cerr << "Unable to get texture resource";
                    return nullptr;
                }

				SetDebugName(d3dResource, filePath.getString());
                SetDebugName(d3dShaderResourceView, filePath.getString(), "ShaderResourceView");

                Render::Texture::Description description;
                description.name = filePath.getString();
                description.width = image.GetMetadata().width;
                description.height = image.GetMetadata().height;
                description.depth = image.GetMetadata().depth;
                description.format = Direct3D11::GetFormat(image.GetMetadata().format);
                description.mipMapCount = image.GetMetadata().mipLevels;
                return std::make_unique<Texture>(description, d3dResource, d3dShaderResourceView);
            }

            Render::QueuePtr createQueue(Queue::Type type)
            {
                return std::make_unique<Queue>(this, d3dDefaultContext);
            }

            Render::CommandListPtr createCommandList(uint32_t flags)
            {
                CComPtr<ID3D11DeviceContext> d3dDeferredContext;
                HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredContext);
                if (!d3dDeferredContext)
                {
                    std::cerr << "Unable to create deferred render queue";
                    return nullptr;
                }

                //SetDebugName(d3dDeviceContext, name);
                return std::make_unique<CommandList>(this, d3dDeferredContext);
            }

            void present(bool waitForVerticalSync)
            {
                assert(dxgiSwapChain);

                dxgiSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Direct3D11
}; // namespace Gek
