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
#include <dxgi1_6.h>
#include <algorithm>
#include <comdef.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <memory>
#include <ppl.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
    namespace Render
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

		static constexpr D3D12_DEPTH_WRITE_MASK DepthWriteMaskList[] =
        {
            D3D12_DEPTH_WRITE_MASK_ZERO,
            D3D12_DEPTH_WRITE_MASK_ALL,
        };

		static constexpr D3D12_TEXTURE_ADDRESS_MODE AddressModeList[] =
        {
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_MIRROR,
            D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE,
            D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        };

        static constexpr D3D12_COMPARISON_FUNC ComparisonFunctionList[] =
        {
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_COMPARISON_FUNC_NEVER,
            D3D12_COMPARISON_FUNC_EQUAL,
            D3D12_COMPARISON_FUNC_NOT_EQUAL,
            D3D12_COMPARISON_FUNC_LESS,
            D3D12_COMPARISON_FUNC_LESS_EQUAL,
            D3D12_COMPARISON_FUNC_GREATER,
            D3D12_COMPARISON_FUNC_GREATER_EQUAL,
        };

		static constexpr D3D12_STENCIL_OP StencilOperationList[] =
        {
            D3D12_STENCIL_OP_ZERO,
            D3D12_STENCIL_OP_KEEP,
            D3D12_STENCIL_OP_REPLACE,
            D3D12_STENCIL_OP_INVERT,
            D3D12_STENCIL_OP_INCR,
            D3D12_STENCIL_OP_INCR_SAT,
            D3D12_STENCIL_OP_DECR,
            D3D12_STENCIL_OP_DECR_SAT,
        };

		static constexpr D3D12_BLEND BlendSourceList[] =
        {
            D3D12_BLEND_ZERO,
            D3D12_BLEND_ONE,
            D3D12_BLEND_BLEND_FACTOR,
            D3D12_BLEND_INV_BLEND_FACTOR,
            D3D12_BLEND_SRC_COLOR,
            D3D12_BLEND_INV_SRC_COLOR,
            D3D12_BLEND_SRC_ALPHA,
            D3D12_BLEND_INV_SRC_ALPHA,
            D3D12_BLEND_SRC_ALPHA_SAT,
            D3D12_BLEND_DEST_COLOR,
            D3D12_BLEND_INV_DEST_COLOR,
            D3D12_BLEND_DEST_ALPHA,
            D3D12_BLEND_INV_DEST_ALPHA,
            D3D12_BLEND_SRC1_COLOR,
            D3D12_BLEND_INV_SRC1_COLOR,
            D3D12_BLEND_SRC1_ALPHA,
            D3D12_BLEND_INV_SRC1_ALPHA,
        };

		static constexpr D3D12_BLEND_OP BlendOperationList[] =
        {
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_OP_SUBTRACT,
            D3D12_BLEND_OP_REV_SUBTRACT,
            D3D12_BLEND_OP_MIN,
            D3D12_BLEND_OP_MAX,
        };

		static constexpr D3D12_FILL_MODE FillModeList[] =
        {
            D3D12_FILL_MODE_WIREFRAME,
            D3D12_FILL_MODE_SOLID,
        };

		static constexpr D3D12_CULL_MODE CullModeList[] =
        {
            D3D12_CULL_MODE_NONE,
            D3D12_CULL_MODE_FRONT,
            D3D12_CULL_MODE_BACK,
        };

		static constexpr D3D12_FILTER FilterList[] =
        {
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
            D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
            D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_FILTER_ANISOTROPIC,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
            D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
            D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
            D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
            D3D12_FILTER_COMPARISON_ANISOTROPIC,
            D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT,
            D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
            D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
            D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR,
            D3D12_FILTER_MINIMUM_ANISOTROPIC,
            D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT,
            D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
            D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
            D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
            D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
            D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR,
            D3D12_FILTER_MAXIMUM_ANISOTROPIC,
        };

		static constexpr D3D12_PRIMITIVE_TOPOLOGY TopologList[] =
        {
            D3D_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
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

        GEK_CONTEXT_USER(Implementation, Window*, Render::Device::Description)
            , public Render::Debug::Device
        {
            struct PipelineFormat
                : public Render::PipelineFormat
            {
            public:
                Render::PipelineFormat::Description description;
                ID3D12RootSignature* d3dRootSignature = nullptr;

            public:
                PipelineFormat(const Render::PipelineFormat::Description& description)
                    : description(description)
                {
                }

                virtual ~PipelineFormat(void)
                {
                    AtomicRelease(&d3dRootSignature);
                }

                std::string_view getName(void) const
                {
                    return description.name;
                }

                Render::PipelineFormat::Description const& getDescription(void) const
                {
                    return description;
                }
            };

            struct PipelineState
                : public Render::PipelineState
            {
            public:
                Render::PipelineState::Description description;
                ID3D12PipelineState* d3dPipelineState = nullptr;
                D3D12_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology;
                uint32_t d3dSampleMask = 0x0;

            public:
                PipelineState(const Render::PipelineState::Description& description,
                    CComPtr<ID3D12PipelineState>& d3dPipelineState,
                    D3D11_PRIMITIVE_TOPOLOGY d3dPrimitiveTopology,
                    uint32_t d3dSampleMask)
                    : description(description)
                {
                    AtomicAddRef(&this->d3dPipelineState, d3dPipelineState);
                }

                virtual ~PipelineState(void)
                {
                    AtomicRelease(&d3dPipelineState);
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
                ID3D12Resource* d3dSamplerState = nullptr;

            public:
                SamplerState(const Render::SamplerState::Description& description, CComPtr<ID3D12Resource>& d3dSamplerState)
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
                ID3D12Resource* d3dShaderResourceView = nullptr;

            public:
                ViewResource(CComPtr<ID3D12Resource>& d3dShaderResourceView)
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
                ID3D12Resource* d3dUnorderedAccessView = nullptr;

            public:
                UnorderedResource(CComPtr<ID3D12Resource>& d3dUnorderedAccessView)
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
                ID3D12Resource* d3dRenderTargetView = nullptr;

            public:
                TargetResource(CComPtr<ID3D12Resource>& d3dRenderTargetView)
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
                ID3D12Resource* d3dBuffer = nullptr;

            public:
                Buffer(const Render::Buffer::Description& description,
                    CComPtr<ID3D12Resource>& d3dBuffer,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView,
                    CComPtr<ID3D12Resource>& d3dUnorderedAccessView)
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
                ID3D12Resource* d3dBaseResource = nullptr;

            public:
                Texture(const Render::Texture::Description& description,
                    CComPtr<ID3D12Resource>& d3dBaseResource,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView)
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
                    CComPtr<ID3D12Resource>& d3dBaseResource,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView,
                    CComPtr<ID3D12Resource>& d3dUnorderedAccessView)
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
                    CComPtr<ID3D12Resource>& d3dBaseResource,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView,
                    CComPtr<ID3D12Resource>& d3dUnorderedAccessView,
                    CComPtr<ID3D12Resource>& d3dRenderTargetView)
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
                    CComPtr<ID3D12Resource>& d3dBaseResource,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView,
                    CComPtr<ID3D12Resource>& d3dRenderTargetView)
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
                ID3D12Resource* d3dDepthStencilView = nullptr;

            public:
                DepthTexture(const Render::Texture::Description& description,
                    CComPtr<ID3D12Resource>& d3dBaseResource,
                    CComPtr<ID3D12Resource>& d3dShaderResourceView,
                    CComPtr<ID3D12Resource>& d3dUnorderedAccessView,
                    CComPtr<ID3D12Resource> &d3dDepthStencilView)
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
                ID3D12CommandList* d3dCommandList = nullptr;
                ID3D12GraphicsCommandList* d3dGraphicsCommandList = nullptr;

            public:
                CommandList(Device *device)
                    : device(device)
                {
                }

                std::string_view getName(void) const
                {
                    return "command_list";
                }

                // Render::Device::Queue
                void finish(void)
                {
                }

                void generateMipMaps(Render::Resource* texture)
                {
                    auto internalTexture = dynamic_cast<Texture*>(texture);
                    if (internalTexture)
                    {
                    }
                }

                void resolveSamples(Render::Resource* destination, Render::Resource* source)
                {
                    auto internalDestination = dynamic_cast<Texture*>(destination);
                    auto internalSource = dynamic_cast<Texture*>(source);
                    if (internalDestination && internalSource)
                    {
                    }
                }

                void clearUnorderedAccess(Render::Resource* object, Math::Float4 const& value)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalObject = dynamic_cast<UnorderedResource*>(object);
                    if (internalObject)
                    {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
                        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
                        d3dGraphicsCommandList->ClearUnorderedAccessViewFloat(gpuDescriptorHandle, cpuDescriptorHandle, internalObject->d3dUnorderedAccessView, value.data, 0, nullptr);
                    }
                }

                void clearUnorderedAccess(Render::Resource* object, Math::UInt4 const& value)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalObject = dynamic_cast<UnorderedResource*>(object);
                    if (internalObject)
                    {
                        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
                        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
                        d3dGraphicsCommandList->ClearUnorderedAccessViewUint(gpuDescriptorHandle, cpuDescriptorHandle, internalObject->d3dUnorderedAccessView, value.data, 0, nullptr);
                    }
                }

                void clearRenderTarget(Render::Texture* renderTarget, Math::Float4 const& clearColor)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalTarget = dynamic_cast<TargetResource*>(renderTarget);
                    if (internalTarget)
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
                        d3dGraphicsCommandList->ClearRenderTargetView(cpuDescriptorHandle, clearColor.data, 0, nullptr);
                    }
                }

                void clearDepthStencilTarget(Render::Texture* depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalBuffer = dynamic_cast<DepthTexture*>(depthBuffer);
                    if (internalBuffer)
                    {
                        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
                        d3dGraphicsCommandList->ClearDepthStencilView(cpuDescriptorHandle,
                            (D3D12_CLEAR_FLAGS)((flags & Render::ClearFlags::Depth ? D3D12_CLEAR_FLAG_DEPTH : 0) |
                                                (flags & Render::ClearFlags::Stencil ? D3D12_CLEAR_FLAG_STENCIL : 0)),
                            clearDepth, clearStencil, 0, nullptr);
                    }
                }

                void setViewportList(const std::vector<Render::ViewPort>& viewPortList)
                {
                    assert(d3dGraphicsCommandList);

                    d3dGraphicsCommandList->RSSetViewports(viewPortList.size(), (D3D12_VIEWPORT *)viewPortList.data());
                }

                void setScissorList(const std::vector<Math::UInt4>& rectangleList)
                {
                    assert(d3dGraphicsCommandList);

                    d3dGraphicsCommandList->RSSetScissorRects(rectangleList.size(), (D3D12_RECT *)rectangleList.data());
                }

                void bindPipelineState(Render::PipelineState* pipelineState)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalPipelineState = dynamic_cast<PipelineState*>(pipelineState);
                    if (internalPipelineState)
                    {
                        d3dGraphicsCommandList->SetPipelineState(internalPipelineState->d3dPipelineState);
                        d3dGraphicsCommandList->IASetPrimitiveTopology(internalPipelineState->d3dPrimitiveTopology);
                    }
                }

                void bindSamplerStateList(const std::vector<Render::SamplerState*>& samplerStateList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dSamplerStateList;
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
                        //d3dGraphicsCommandList->VSSetSamplers(firstStage, d3dSamplerStateList.size(), d3dSamplerStateList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        //d3dGraphicsCommandList->PSSetSamplers(firstStage, d3dSamplerStateList.size(), d3dSamplerStateList.data());
                    }
                }

                void bindConstantBufferList(const std::vector<Render::Buffer*>& constantBufferList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dconstantBufferList;
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
                        //d3dGraphicsCommandList->VSSetConstantBuffers(firstStage, d3dconstantBufferList.size(), d3dconstantBufferList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        //d3dGraphicsCommandList->PSSetConstantBuffers(firstStage, d3dconstantBufferList.size(), d3dconstantBufferList.data());
                    }
                }

                void bindResourceList(const std::vector<Render::Resource*>& resourceList, uint32_t firstStage, uint8_t pipelineFlags)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dResourceList;
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
                        //d3dGraphicsCommandList->VSSetShaderResources(firstStage, d3dResourceList.size(), d3dResourceList.data());
                    }

                    if (pipelineFlags & Render::Pipeline::Pixel)
                    {
                        //d3dGraphicsCommandList->PSSetShaderResources(firstStage, d3dResourceList.size(), d3dResourceList.data());
                    }
                }

                void bindUnorderedAccessList(const std::vector<Render::Resource*>& unorderedResourceList, uint32_t firstStage, uint32_t* countList)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dResourceList;
                    for (auto& unorderedResource : unorderedResourceList)
                    {
                        auto internalResource = dynamic_cast<UnorderedResource*>(unorderedResource);
                        if (internalResource)
                        {
                            d3dResourceList.push_back(internalResource->d3dUnorderedAccessView);
                        }
                    }

                    //d3dGraphicsCommandList->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, d3dResourceList.size(), d3dResourceList.data(), countList);
                }

                void bindIndexBuffer(Render::Resource* indexBuffer, uint32_t offset)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalBuffer = dynamic_cast<Buffer*>(indexBuffer);
                    if (internalBuffer)
                    {
                        DXGI_FORMAT format = BufferFormatList[static_cast<uint8_t>(internalBuffer->description.format)];

                        D3D12_INDEX_BUFFER_VIEW bufferView;
                        d3dGraphicsCommandList->IASetIndexBuffer(&bufferView);
                    }
                }

                void bindVertexBufferList(const std::vector<Render::Resource*>& vertexBufferList, uint32_t firstSlot, uint32_t* offsetList)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dBufferList;
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

                    D3D12_VERTEX_BUFFER_VIEW bufferView;
                    d3dGraphicsCommandList->IASetVertexBuffers(firstSlot, d3dBufferList.size(), &bufferView);
                }

                void bindRenderTargetList(const std::vector<Render::Texture*>& renderTargetList, Render::Texture* depthBuffer)
                {
                    assert(d3dGraphicsCommandList);

                    std::vector<ID3D12Resource*> d3dTargetList;
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
                        D3D12_CPU_DESCRIPTOR_HANDLE handle;
                        //d3dGraphicsCommandList->OMSetRenderTargets(d3dTargetList.size(), d3dTargetList.data(), internalDepth->d3dDepthStencilView);
                    }
                    else
                    {
                        //d3dGraphicsCommandList->OMSetRenderTargets(d3dTargetList.size(), d3dTargetList.data(), nullptr);
                    }
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    assert(d3dGraphicsCommandList);

                    d3dGraphicsCommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    assert(d3dGraphicsCommandList);

                    d3dGraphicsCommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                    assert(d3dGraphicsCommandList);

                    d3dGraphicsCommandList->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }

                void drawInstancedPrimitive(Render::Resource* bufferArguments)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        //d3dGraphicsCommandList->DrawIndexedInstanced(internalBuffer->d3dBuffer, 0);
                    }
                }

                void drawInstancedIndexedPrimitive(Render::Resource* bufferArguments)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        //d3dGraphicsCommandList->DrawIndexedInstancedIndirect(internalBuffer->d3dBuffer, 0);
                    }
                }

                void dispatch(Render::Resource* bufferArguments)
                {
                    assert(d3dGraphicsCommandList);

                    auto internalBuffer = dynamic_cast<Buffer*>(bufferArguments);
                    if (internalBuffer)
                    {
                        //d3dGraphicsCommandList->DispatchIndirect(internalBuffer->d3dBuffer, 0);
                    }
                }
            };

            class Queue
                : public Render::Queue
            {
            public:
                Device* device = nullptr;
                ID3D12CommandQueue* d3dCommandQueue = nullptr;

            public:
                Queue(Device *device, CComPtr<ID3D12CommandQueue> &d3dCommandQueue)
                {
                    AtomicAddRef(&this->d3dCommandQueue, d3dCommandQueue);
                }

                virtual ~Queue(void)
                {
                    AtomicRelease(&d3dCommandQueue);
                }

                std::string_view getName(void) const
                {
                    return "queue";
                }

                void executeCommandList(Render::CommandList *commandList)
                {
                    assert(d3dCommandQueue);
                    assert(commandList);

                    auto internalList = dynamic_cast<CommandList*>(commandList);
                    if (internalList)
                    {
                        d3dCommandQueue->ExecuteCommandLists(1, &internalList->d3dCommandList);
                    }
                }
            };


        public:
            Window *window = nullptr;
            bool isChildWindow = false;

            CComPtr<ID3D12Device> d3dDevice;
            CComPtr<IDXGISwapChain> dxgiSwapChain;

            static const uint32_t BackBufferCount = 3;

            CComPtr<ID3D12Resource> d3dRenderTargets[BackBufferCount];
            CComPtr<ID3D12CommandQueue> d3dCommandQueue;

            HANDLE d3dFrameFenceEvents[BackBufferCount];
            CComPtr<ID3D12Fence> d3dFrameFences[BackBufferCount];
            uint64_t d3dCurrentFenceValue = 0;
            uint64_t d3dFenceValues[BackBufferCount];

            CComPtr<ID3D12DescriptorHeap> d3dRenderTargetDescriptorHeap;

            CComPtr<ID3D12RootSignature> d3dRootSignature;
            CComPtr<ID3D12PipelineState> d3dPipelineState;

            CComPtr<ID3D12CommandAllocator> d3dCommandAllocators[BackBufferCount];
            CComPtr<ID3D12GraphicsCommandList> d3dCommandLists[BackBufferCount];

            int d3dCurrentBackBuffer = 0;

            std::int32_t d3dRenderTargetViewDescriptorSize;

        public:
            Implementation(Gek::Context *context, Window *window, Render::Device::Description deviceDescription)
                : ContextRegistration(context)
                , window(window)
                , isChildWindow(GetParent((HWND)window->getBaseWindow()) != nullptr)
            {
                assert(window);

                UINT createFactoryFlags = 0;
#if _DEBUG
                createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

                CComPtr<IDXGIFactory6> dxgiFactory;
                HRESULT resultValue = CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
                if (FAILED(resultValue) || !dxgiFactory)
                {
                    throw std::runtime_error("Unable to get graphics factory");
                }

                CComPtr<IDXGIAdapter> dxgiAdapter;
                for(uint32_t adapterIndex = 0; dxgiFactory->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&dxgiAdapter)) != DXGI_ERROR_NOT_FOUND; ++adapterIndex)
                {
                    if (SUCCEEDED(D3D12CreateDevice(dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3dDevice))))
                    {
                        break;
                    }
                }

                if (!d3dDevice)
                {
                    throw std::runtime_error("Unable to find GPU adapter");
                }

#ifdef _DEBUG
                CComQIPtr<ID3D12Debug> d3dDebug(d3dDevice);
                D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug));
                d3dDebug->EnableDebugLayer();

                //CComQIPtr<ID3D12InfoQueue> d3dInfoQueue(d3dDebug);
                //d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                //d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                //d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
#endif
                DXGI_SWAP_CHAIN_DESC swapChainDescription;
                swapChainDescription.BufferCount = 2;
                swapChainDescription.BufferDesc.Format = Render::TextureFormatList[static_cast<uint8_t>(deviceDescription.displayFormat)];
                swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
                swapChainDescription.BufferDesc.Width = 300;
                swapChainDescription.BufferDesc.Height = 200;
                swapChainDescription.OutputWindow = (HWND)window->getBaseWindow();
                swapChainDescription.SampleDesc.Count = 1;
                swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
                swapChainDescription.Windowed = true;

                resultValue = dxgiFactory->CreateSwapChain(d3dDevice, &swapChainDescription, &dxgiSwapChain);
                if (FAILED(resultValue) || !dxgiSwapChain)
                {
                    throw std::runtime_error("Unable to create swap chain for window");
                }

                dxgiFactory->MakeWindowAssociation((HWND)window->getBaseWindow(), 0);

                for (uint32_t backBuffer = 0; backBuffer < BackBufferCount; ++backBuffer)
                {
                    d3dFrameFenceEvents[backBuffer] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                    d3dFenceValues[backBuffer] = 0;
                    d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3dFrameFences[backBuffer]));
                }

                D3D12_DESCRIPTOR_HEAP_DESC heapDescription = {};
                heapDescription.NumDescriptors = BackBufferCount;
                heapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                heapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                d3dDevice->CreateDescriptorHeap(&heapDescription, IID_PPV_ARGS(&d3dRenderTargetDescriptorHeap));
            }

            ~Implementation(void)
            {
                setFullScreenState(false);
                dxgiSwapChain.Release();
                d3dDevice.Release();
            }

            void createSwapChainDetails(void)
            {
                CD3DX12_CPU_DESCRIPTOR_HANDLE renderTargetHandle
                {
                    d3dRenderTargetDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
                };

                for (uint32_t backBuffer = 0; backBuffer < BackBufferCount; ++backBuffer)
                {
                    dxgiSwapChain->GetBuffer(backBuffer, IID_PPV_ARGS(&d3dRenderTargets[backBuffer]));

                    D3D12_RENDER_TARGET_VIEW_DESC viewDescription;
                    viewDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                    viewDescription.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    viewDescription.Texture2D.MipSlice = 0;
                    viewDescription.Texture2D.PlaneSlice = 0;
                    d3dDevice->CreateRenderTargetView(d3dRenderTargets[backBuffer], &viewDescription, renderTargetHandle);
                    renderTargetHandle.Offset(d3dRenderTargetViewDescriptorSize);
                }
            }

            void deleteSwapChainDetails(void)
            {
                for (uint32_t backBuffer = 0; backBuffer < BackBufferCount; ++backBuffer)
                {
                    d3dRenderTargets[backBuffer] = nullptr;
                }
            }

            std::vector<D3D12_INPUT_ELEMENT_DESC> getVertexDeclaration(const std::vector<Render::PipelineState::VertexDeclaration> &vertexDeclaration)
            {
                uint32_t semanticIndexList[static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count)] = { 0 };
                std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescriptionList;
                for (auto const &vertexElement : vertexDeclaration)
                {
                    D3D12_INPUT_ELEMENT_DESC elementDescription;
                    elementDescription.Format = Render::BufferFormatList[static_cast<uint8_t>(vertexElement.format)];
                    elementDescription.AlignedByteOffset = (vertexElement.alignedByteOffset == Render::PipelineState::VertexDeclaration::AppendAligned ? D3D12_APPEND_ALIGNED_ELEMENT : vertexElement.alignedByteOffset);
                    elementDescription.SemanticName = Render::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)].data();
                    elementDescription.SemanticIndex = semanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++;
                    elementDescription.InputSlot = vertexElement.sourceIndex;
                    switch (vertexElement.source)
                    {
                    case Render::PipelineState::VertexDeclaration::Source::Instance:
                        elementDescription.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                        elementDescription.InstanceDataStepRate = 1;
                        break;

                    case Render::PipelineState::VertexDeclaration::Source::Vertex:
                    default:
                        elementDescription.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
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
                    std::string semantic = Render::VertexSemanticList[static_cast<uint8_t>(vertexElement.semantic)].data();
                    std::string format = Render::getFormatSemantic(vertexElement.format);
                    vertexData.push_back(std::format("    {} {} : {}{};", format, vertexElement.name, semantic, vertexSemanticIndexList[static_cast<uint8_t>(vertexElement.semantic)]++));
                }

                std::vector<std::string> pixelData;
                uint32_t pixelSemanticIndexList[static_cast<uint8_t>(Render::PipelineState::ElementDeclaration::Semantic::Count)] = { 0 };
                for (auto const &pixelElement : pipelineStateDescription.pixelDeclaration)
                {
                    std::string semantic = Render::PixelSemanticList[static_cast<uint8_t>(pixelElement.semantic)].data();
                    std::string format = Render::getFormatSemantic(pixelElement.format);
                    pixelData.push_back(std::format("    {} {} : {}{};", format, pixelElement.name, semantic, pixelSemanticIndexList[static_cast<uint8_t>(pixelElement.semantic)]++));
                }

                uint32_t renderTargetIndex = 0;
                std::vector<std::string> renderTargets;
                for (auto const &renderTarget : pipelineStateDescription.renderTargetList)
                {
                    std::string format = Render::getFormatSemantic(renderTarget.format);
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
            Render::Texture* getBackBuffer(void)
            {
                return nullptr;
            }

            Render::DisplayModeList getDisplayModeList(Render::Format format) const
            {
                Render::DisplayModeList displayModeList;

                CComPtr<IDXGIOutput> dxgiOutput;
                dxgiSwapChain->GetContainingOutput(&dxgiOutput);
                if (dxgiOutput)
                {
                    uint32_t modeCount = 0;
                    dxgiOutput->GetDisplayModeList(Render::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, nullptr);

                    std::vector<DXGI_MODE_DESC> dxgiDisplayModeList(modeCount);
                    dxgiOutput->GetDisplayModeList(Render::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, dxgiDisplayModeList.data());

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
                            displayMode.format = Render::GetFormat(dxgiDisplayMode.Format);
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
                assert(dxgiSwapChain);

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

                DXGI_MODE_DESC description;
                description.Width = displayMode.width;
                description.Height = displayMode.height;
                description.RefreshRate.Numerator = displayMode.refreshRate.numerator;
                description.RefreshRate.Denominator = displayMode.refreshRate.denominator;
                description.Format = Render::TextureFormatList[static_cast<uint8_t>(displayMode.format)];
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

                HRESULT resultValue = dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
                if (FAILED(resultValue))
                {
                    throw std::runtime_error("Unable to resize swap chain buffers to window size");
                }
            }

            Render::PipelineFormatPtr createPipelineFormat(const Render::PipelineFormat::Description& pipelineDescription)
            {
                return std::make_unique<PipelineFormat>(pipelineDescription);
            }

            Render::PipelineStatePtr createPipelineState(Render::PipelineFormat* pipelineFormat, const Render::PipelineState::Description &pipelineStateDescription)
            {
                auto d3dPipelineFormat = dynamic_cast<PipelineFormat*>(pipelineFormat);

                auto shaderHeader = getShaderHeader(pipelineStateDescription);
                auto compiledVertexShader = compileShader("", "vs_5_0", pipelineStateDescription.vertexShaderEntryFunction, pipelineStateDescription.vertexShader, shaderHeader);
                auto compiledPixelShader = compileShader("", "ps_5_0", pipelineStateDescription.pixelShaderEntryFunction, pipelineStateDescription.pixelShader, shaderHeader);
                auto vertexDeclaration = getVertexDeclaration(pipelineStateDescription.vertexDeclaration);
                auto d3dPrimitiveTopology = Render::TopologList[static_cast<uint8_t>(pipelineStateDescription.primitiveType)];
                auto d3dSampleMask = pipelineStateDescription.sampleMask;

                static const D3D_SHADER_MACRO macros[] = {
                    { "D3D12_SAMPLE_TEXTURE", "1" },
                    { nullptr, nullptr }
                };

                D3D12_GRAPHICS_PIPELINE_STATE_DESC d3dDescription = {};
                d3dDescription.VS.BytecodeLength = compiledVertexShader.size();
                d3dDescription.VS.pShaderBytecode = compiledVertexShader.data();
                d3dDescription.PS.BytecodeLength = compiledPixelShader.size();
                d3dDescription.PS.pShaderBytecode = compiledPixelShader.data();
                d3dDescription.pRootSignature = d3dPipelineFormat->d3dRootSignature;
                d3dDescription.NumRenderTargets = 1;
                d3dDescription.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                d3dDescription.DSVFormat = DXGI_FORMAT_UNKNOWN;
                d3dDescription.InputLayout.NumElements = vertexDeclaration.size();
                d3dDescription.InputLayout.pInputElementDescs = vertexDeclaration.data();
                d3dDescription.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
                d3dDescription.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
                // Simple alpha blending
                d3dDescription.BlendState.RenderTarget[0].BlendEnable = true;
                d3dDescription.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
                d3dDescription.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                d3dDescription.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
                d3dDescription.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
                d3dDescription.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
                d3dDescription.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
                d3dDescription.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                d3dDescription.SampleDesc.Count = 1;
                d3dDescription.DepthStencilState.DepthEnable = false;
                d3dDescription.DepthStencilState.StencilEnable = false;
                d3dDescription.SampleMask = d3dSampleMask;
                d3dDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

                CComPtr<ID3D12PipelineState> d3dPipelineState;
                HRESULT retVal = d3dDevice->CreateGraphicsPipelineState(&d3dDescription, IID_PPV_ARGS(&d3dPipelineState));
                return std::make_unique<PipelineState>(pipelineStateDescription, d3dPipelineState, d3dPrimitiveTopology, d3dSampleMask);
            }

            Render::SamplerStatePtr createSamplerState(Render::PipelineFormat* pipelineFormat, const Render::SamplerState::Description &samplerStateDescription)
            {
                D3D12_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = Render::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeU)];
                samplerDescription.AddressV = Render::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeV)];
                samplerDescription.AddressW = Render::AddressModeList[static_cast<uint8_t>(samplerStateDescription.addressModeW)];
                samplerDescription.MipLODBias = samplerStateDescription.mipLevelBias;
                samplerDescription.MaxAnisotropy = samplerStateDescription.maximumAnisotropy;
                samplerDescription.ComparisonFunc = Render::ComparisonFunctionList[static_cast<uint8_t>(samplerStateDescription.comparisonFunction)];
                samplerDescription.BorderColor[0] = samplerStateDescription.borderColor.r;
                samplerDescription.BorderColor[1] = samplerStateDescription.borderColor.g;
                samplerDescription.BorderColor[2] = samplerStateDescription.borderColor.b;
                samplerDescription.BorderColor[3] = samplerStateDescription.borderColor.a;
                samplerDescription.MinLOD = samplerStateDescription.minimumMipLevel;
                samplerDescription.MaxLOD = samplerStateDescription.maximumMipLevel;
                samplerDescription.Filter = Render::FilterList[static_cast<uint8_t>(samplerStateDescription.filterMode)];

                CComPtr<ID3D12Resource> d3dSamplerState;
                return std::make_unique<SamplerState>(samplerStateDescription, d3dSamplerState);
            }

            bool mapResource(Render::Resource* resource, void *&data, Render::Map mapping)
            {
                return false;
            }

            void unmapResource(Render::Resource* resource)
            {
            }

            void updateResource(Render::Resource* resource, const void *data)
            {
            }

            void copyResource(Render::Resource* destination, Render::Resource* source)
            {
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

                    stride = Render::FormatStrideList[static_cast<uint8_t>(description.format)];
                }
                else if (description.stride == 0)
                {
                    std::cerr << "Buffer requires either a format or an element stride";
                    return nullptr;
                }

                return nullptr;
            }

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data)
            {
                return nullptr;
            }

            Render::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                return nullptr;
            }

            Render::QueuePtr createQueue(Render::Queue::Type type)
            {
                CComPtr<ID3D12CommandQueue> d3dCommandQueue;
                return std::make_unique<Queue>(this, d3dCommandQueue);
            }

            Render::CommandListPtr createCommandList(uint32_t flags)
            {
                return std::make_unique<CommandList>(this);
            }

            void present(bool waitForVerticalSync)
            {
                assert(dxgiSwapChain);

                dxgiSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);

                const auto fenceValue = d3dCurrentFenceValue;
                d3dCommandQueue->Signal(d3dFrameFences[d3dCurrentBackBuffer], fenceValue);
                d3dFenceValues[d3dCurrentBackBuffer] = fenceValue;
                ++d3dCurrentFenceValue;

                d3dCurrentBackBuffer = (d3dCurrentBackBuffer + 1) % BackBufferCount;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Implementation);
    }; // Render
}; // namespace Gek