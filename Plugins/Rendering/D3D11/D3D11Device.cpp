#pragma warning(disable : 4005)

#include "API/System/RenderDevice.hpp"
#include "API/System/WindowDevice.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/String.hpp"
#include <ktx.h>
#define STB_IMAGE_IMPLEMENTATION
#include <D3D11Shader.h>
#include <algorithm>
#include <atlbase.h>
#include <chrono>
#include <comdef.h>
#include <ctime>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_3.h>
#include <execution>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <ppl.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <sstream>
#include <stb_image.h>
#include <wincodec.h>

extern "C" const IID IID_ID3D11ShaderReflection = { 0x8d536ca1, 0x0cca, 0x4956, { 0xa8, 0x37, 0x78, 0x08, 0x49, 0xae, 0xd6, 0x51 } };

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace Gek
{
    namespace Render::Implementation
    {
        // All these lists must match, since the same GEK Format can be used for either textures or buffers
        // The size list must also match
        static constexpr DXGI_FORMAT TextureFormatList[] = {
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

        static constexpr DXGI_FORMAT DepthFormatList[] = {
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

        static constexpr DXGI_FORMAT ViewFormatList[] = {
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

        static constexpr DXGI_FORMAT BufferFormatList[] = {
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

        static constexpr uint32_t FormatStrideList[] = {
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
            (sizeof(uint8_t) * 4),  // DXGI_FORMAT_R8G8B8A8_UINT,
            (sizeof(uint32_t) * 3), // DXGI_FORMAT_R32G32B32_UINT,
            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32G32_UINT,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_UINT,
            (sizeof(uint8_t) * 2),  // DXGI_FORMAT_R8G8_UINT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_UINT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UINT,
            (sizeof(uint8_t) * 1),  // DXGI_FORMAT_R8_UINT,

            (sizeof(uint32_t) * 4), // DXGI_FORMAT_R32G32B32A32_SINT,
            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_SINT,
            (sizeof(uint8_t) * 4),  // DXGI_FORMAT_R8G8B8A8_SINT,
            (sizeof(uint32_t) * 3), // DXGI_FORMAT_R32G32B32_SINT,
            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32G32_SINT,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_SINT,
            (sizeof(uint8_t) * 2),  // DXGI_FORMAT_R8G8_SINT,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_SINT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_SINT,
            (sizeof(uint8_t) * 1),  // DXGI_FORMAT_R8_SINT,

            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_UNORM,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R10G10B10A2_UNORM,
            (sizeof(uint8_t) * 4),  // DXGI_FORMAT_R8G8B8A8_UNORM,
            (sizeof(uint8_t) * 4),  // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_UNORM,
            (sizeof(uint8_t) * 2),  // DXGI_FORMAT_R8G8_UNORM,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UNORM,
            (sizeof(uint8_t) * 1),  // DXGI_FORMAT_R8_UNORM,

            (sizeof(uint16_t) * 4), // DXGI_FORMAT_R16G16B16A16_SNORM,
            (sizeof(uint8_t) * 4),  // DXGI_FORMAT_R8G8B8A8_SNORM,
            (sizeof(uint16_t) * 2), // DXGI_FORMAT_R16G16_SNORM,
            (sizeof(uint8_t) * 2),  // DXGI_FORMAT_R8G8_SNORM,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_SNORM,
            (sizeof(uint8_t) * 1),  // DXGI_FORMAT_R8_SNORM,

            (sizeof(uint32_t) * 2), // DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R24_UNORM_X8_TYPELESS,

            (sizeof(uint32_t) * 1), // DXGI_FORMAT_R32_FLOAT,
            (sizeof(uint16_t) * 1), // DXGI_FORMAT_R16_UNORM,
        };

        static_assert(ARRAYSIZE(FormatStrideList) == static_cast<uint8_t>(Render::Format::Count), "New format added without adding to all FormatStrideList.");

        static constexpr D3D11_QUERY QueryList[] = {
            D3D11_QUERY_EVENT,
            D3D11_QUERY_TIMESTAMP,
            D3D11_QUERY_TIMESTAMP_DISJOINT,
        };

        static_assert(ARRAYSIZE(QueryList) == static_cast<uint8_t>(Render::Query::Type::Count), "New query type added without adding to QueryList.");

        static constexpr D3D11_DEPTH_WRITE_MASK DepthWriteMaskList[] = {
            D3D11_DEPTH_WRITE_MASK_ZERO,
            D3D11_DEPTH_WRITE_MASK_ALL,
        };

        static constexpr D3D11_TEXTURE_ADDRESS_MODE AddressModeList[] = {
            D3D11_TEXTURE_ADDRESS_CLAMP,
            D3D11_TEXTURE_ADDRESS_WRAP,
            D3D11_TEXTURE_ADDRESS_MIRROR,
            D3D11_TEXTURE_ADDRESS_MIRROR_ONCE,
            D3D11_TEXTURE_ADDRESS_BORDER,
        };

        static constexpr D3D11_COMPARISON_FUNC ComparisonFunctionList[] = {
            D3D11_COMPARISON_ALWAYS,
            D3D11_COMPARISON_NEVER,
            D3D11_COMPARISON_EQUAL,
            D3D11_COMPARISON_NOT_EQUAL,
            D3D11_COMPARISON_LESS,
            D3D11_COMPARISON_LESS_EQUAL,
            D3D11_COMPARISON_GREATER,
            D3D11_COMPARISON_GREATER_EQUAL,
        };

        static constexpr D3D11_STENCIL_OP StencilOperationList[] = {
            D3D11_STENCIL_OP_ZERO,
            D3D11_STENCIL_OP_KEEP,
            D3D11_STENCIL_OP_REPLACE,
            D3D11_STENCIL_OP_INVERT,
            D3D11_STENCIL_OP_INCR,
            D3D11_STENCIL_OP_INCR_SAT,
            D3D11_STENCIL_OP_DECR,
            D3D11_STENCIL_OP_DECR_SAT,
        };

        static constexpr D3D11_BLEND BlendSourceList[] = {
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

        static constexpr D3D11_BLEND_OP BlendOperationList[] = {
            D3D11_BLEND_OP_ADD,
            D3D11_BLEND_OP_SUBTRACT,
            D3D11_BLEND_OP_REV_SUBTRACT,
            D3D11_BLEND_OP_MIN,
            D3D11_BLEND_OP_MAX,
        };

        static constexpr D3D11_FILL_MODE FillModeList[] = {
            D3D11_FILL_WIREFRAME,
            D3D11_FILL_SOLID,
        };

        static constexpr D3D11_CULL_MODE CullModeList[] = {
            D3D11_CULL_NONE,
            D3D11_CULL_FRONT,
            D3D11_CULL_BACK,
        };

        static constexpr D3D11_FILTER FilterList[] = {
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

        static constexpr D3D11_PRIMITIVE_TOPOLOGY TopologList[] = {
            D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
            D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
        };

        static constexpr D3D11_MAP MapList[] = {
            D3D11_MAP_READ,
            D3D11_MAP_WRITE,
            D3D11_MAP_READ_WRITE,
            D3D11_MAP_WRITE_DISCARD,
            D3D11_MAP_WRITE_NO_OVERWRITE,
        };

        static constexpr std::string_view SemanticNameList[] = {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMAL",
            "NORMAL",
            "COLOR",
        };

        static_assert(ARRAYSIZE(SemanticNameList) == static_cast<uint8_t>(Render::InputElement::Semantic::Count), "New input element semantic added without adding to all SemanticNameList.");

        std::string getHRESULTString(HRESULT resultValue)
        {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << static_cast<uint32_t>(resultValue);
            return stream.str();
        }

        void logD3D11Failure(Gek::Context *context, ID3D11Device *d3dDevice, std::string_view operation, HRESULT resultValue)
        {
            std::string message = std::format("D3D11 {} failed ({}): {}", operation, getHRESULTString(resultValue), _com_error(resultValue).ErrorMessage());

            if (resultValue == DXGI_ERROR_DEVICE_REMOVED ||
                resultValue == DXGI_ERROR_DEVICE_HUNG ||
                resultValue == DXGI_ERROR_DEVICE_RESET ||
                resultValue == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
            {
                HRESULT removedReason = (d3dDevice ? d3dDevice->GetDeviceRemovedReason() : E_FAIL);
                message += std::format(" [GetDeviceRemovedReason={}: {}]", getHRESULTString(removedReason), _com_error(removedReason).ErrorMessage());
            }

            if (context)
            {
                context->log(Gek::Context::Error, "{}", message);
            }
        }

        bool validateTextureForD3D11(Gek::Context *context, ID3D11Device *d3dDevice, DXGI_FORMAT format, uint32_t width, uint32_t height)
        {
            if (!d3dDevice)
            {
                if (context)
                {
                    context->log(Gek::Context::Error, "D3D11 texture validation failed: device is null");
                }
                return false;
            }

            if (format == DXGI_FORMAT_UNKNOWN || width == 0 || height == 0)
            {
                if (context)
                {
                    context->log(Gek::Context::Error, "D3D11 texture validation failed: invalid format or dimensions");
                }
                return false;
            }

            uint32_t formatSupport = 0;
            HRESULT hr = d3dDevice->CheckFormatSupport(format, &formatSupport);
            if (FAILED(hr))
            {
                logD3D11Failure(context, d3dDevice, "CheckFormatSupport", hr);
                return false;
            }

            if (width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
            {
                if (context)
                {
                    context->log(Gek::Context::Error, "D3D11 texture validation failed: dimensions exceed D3D11 limit");
                }
                return false;
            }

            if ((formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) == 0 || (formatSupport & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) == 0)
            {
                if (context)
                {
                    context->log(Gek::Context::Error, "D3D11 texture validation failed: format {} lacks texture2D/sample support", static_cast<int>(format));
                }
                return false;
            }

            return true;
        }

        static DXGI_FORMAT VkFormatToDxgi(uint32_t vkFormat)
        {
            switch (vkFormat)
            {
            case 37:
                return DXGI_FORMAT_R8G8B8A8_UNORM; // VK_FORMAT_R8G8B8A8_UNORM
            case 43:
                return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // VK_FORMAT_R8G8B8A8_SRGB
            case 131:
                return DXGI_FORMAT_BC1_UNORM; // VK_FORMAT_BC1_RGB_UNORM_BLOCK
            case 132:
                return DXGI_FORMAT_BC1_UNORM_SRGB; // VK_FORMAT_BC1_RGB_SRGB_BLOCK
            case 133:
                return DXGI_FORMAT_BC1_UNORM; // VK_FORMAT_BC1_RGBA_UNORM_BLOCK
            case 134:
                return DXGI_FORMAT_BC1_UNORM_SRGB; // VK_FORMAT_BC1_RGBA_SRGB_BLOCK
            case 135:
                return DXGI_FORMAT_BC2_UNORM; // VK_FORMAT_BC2_UNORM_BLOCK
            case 136:
                return DXGI_FORMAT_BC2_UNORM_SRGB; // VK_FORMAT_BC2_SRGB_BLOCK
            case 137:
                return DXGI_FORMAT_BC3_UNORM; // VK_FORMAT_BC3_UNORM_BLOCK
            case 138:
                return DXGI_FORMAT_BC3_UNORM_SRGB; // VK_FORMAT_BC3_SRGB_BLOCK
            case 139:
                return DXGI_FORMAT_BC4_UNORM; // VK_FORMAT_BC4_UNORM_BLOCK
            case 140:
                return DXGI_FORMAT_BC4_SNORM; // VK_FORMAT_BC4_SNORM_BLOCK
            case 141:
                return DXGI_FORMAT_BC5_UNORM; // VK_FORMAT_BC5_UNORM_BLOCK
            case 142:
                return DXGI_FORMAT_BC5_SNORM; // VK_FORMAT_BC5_SNORM_BLOCK
            case 143:
                return DXGI_FORMAT_BC6H_UF16; // VK_FORMAT_BC6H_UFLOAT_BLOCK
            case 144:
                return DXGI_FORMAT_BC6H_SF16; // VK_FORMAT_BC6H_SFLOAT_BLOCK
            case 145:
                return DXGI_FORMAT_BC7_UNORM; // VK_FORMAT_BC7_UNORM_BLOCK
            case 146:
                return DXGI_FORMAT_BC7_UNORM_SRGB; // VK_FORMAT_BC7_SRGB_BLOCK
            default:
                return DXGI_FORMAT_UNKNOWN;
            }
        }

        Render::Format GetFormat(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R32G32B32A32_FLOAT:
                return Render::Format::R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return Render::Format::R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32B32_FLOAT:
                return Render::Format::R32G32B32_FLOAT;
            case DXGI_FORMAT_R11G11B10_FLOAT:
                return Render::Format::R11G11B10_FLOAT;
            case DXGI_FORMAT_R32G32_FLOAT:
                return Render::Format::R32G32_FLOAT;
            case DXGI_FORMAT_R16G16_FLOAT:
                return Render::Format::R16G16_FLOAT;
            case DXGI_FORMAT_R32_FLOAT:
                return Render::Format::R32_FLOAT;
            case DXGI_FORMAT_R16_FLOAT:
                return Render::Format::R16_FLOAT;

            case DXGI_FORMAT_R32G32B32A32_UINT:
                return Render::Format::R32G32B32A32_UINT;
            case DXGI_FORMAT_R16G16B16A16_UINT:
                return Render::Format::R16G16B16A16_UINT;
            case DXGI_FORMAT_R10G10B10A2_UINT:
                return Render::Format::R10G10B10A2_UINT;
            case DXGI_FORMAT_R8G8B8A8_UINT:
                return Render::Format::R8G8B8A8_UINT;
            case DXGI_FORMAT_R32G32B32_UINT:
                return Render::Format::R32G32B32_UINT;
            case DXGI_FORMAT_R32G32_UINT:
                return Render::Format::R32G32_UINT;
            case DXGI_FORMAT_R16G16_UINT:
                return Render::Format::R16G16_UINT;
            case DXGI_FORMAT_R8G8_UINT:
                return Render::Format::R8G8_UINT;
            case DXGI_FORMAT_R32_UINT:
                return Render::Format::R32_UINT;
            case DXGI_FORMAT_R16_UINT:
                return Render::Format::R16_UINT;
            case DXGI_FORMAT_R8_UINT:
                return Render::Format::R8_UINT;

            case DXGI_FORMAT_R32G32B32A32_SINT:
                return Render::Format::R32G32B32A32_INT;
            case DXGI_FORMAT_R16G16B16A16_SINT:
                return Render::Format::R16G16B16A16_INT;
            case DXGI_FORMAT_R8G8B8A8_SINT:
                return Render::Format::R8G8B8A8_INT;
            case DXGI_FORMAT_R32G32B32_SINT:
                return Render::Format::R32G32B32_INT;
            case DXGI_FORMAT_R32G32_SINT:
                return Render::Format::R32G32_INT;
            case DXGI_FORMAT_R16G16_SINT:
                return Render::Format::R16G16_INT;
            case DXGI_FORMAT_R8G8_SINT:
                return Render::Format::R8G8_INT;
            case DXGI_FORMAT_R32_SINT:
                return Render::Format::R32_INT;
            case DXGI_FORMAT_R16_SINT:
                return Render::Format::R16_INT;
            case DXGI_FORMAT_R8_SINT:
                return Render::Format::R8_INT;

            case DXGI_FORMAT_R16G16B16A16_UNORM:
                return Render::Format::R16G16B16A16_UNORM;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                return Render::Format::R10G10B10A2_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return Render::Format::R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return Render::Format::R8G8B8A8_UNORM_SRGB;
            case DXGI_FORMAT_R16G16_UNORM:
                return Render::Format::R16G16_UNORM;
            case DXGI_FORMAT_R8G8_UNORM:
                return Render::Format::R8G8_UNORM;
            case DXGI_FORMAT_R16_UNORM:
                return Render::Format::R16_UNORM;
            case DXGI_FORMAT_R8_UNORM:
                return Render::Format::R8_UNORM;

            case DXGI_FORMAT_R16G16B16A16_SNORM:
                return Render::Format::R16G16B16A16_NORM;
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                return Render::Format::R8G8B8A8_NORM;
            case DXGI_FORMAT_R16G16_SNORM:
                return Render::Format::R16G16_NORM;
            case DXGI_FORMAT_R8G8_SNORM:
                return Render::Format::R8G8_NORM;
            case DXGI_FORMAT_R16_SNORM:
                return Render::Format::R16_NORM;
            case DXGI_FORMAT_R8_SNORM:
                return Render::Format::R8_NORM;
            };

            return Render::Format::Unknown;
        }

        template <typename CONVERT, typename SOURCE>
        auto getObject(SOURCE *source)
        {
            return (source ? dynamic_cast<CONVERT *>(source)->CONVERT::d3dObject : nullptr);
        }

        template <typename TYPE>
        struct ObjectCache
        {
            std::vector<TYPE *> objectList;

            template <typename CONVERT, typename INPUT>
            void set(const std::vector<INPUT> &inputList)
            {
                size_t listCount = inputList.size();
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    objectList[object] = getObject<CONVERT>(inputList[object]);
                }
            }

            void clear(size_t listCount)
            {
                objectList.reserve(std::max(listCount, objectList.size()));
                objectList.resize(listCount);

                for (uint32_t object = 0; object < listCount; ++object)
                {
                    objectList[object] = nullptr;
                }
            }

            TYPE *const *const get(void) const
            {
                return objectList.data();
            }
        };

        template <typename TYPE>
        class BaseObject
        {
          public:
            TYPE *d3dObject = nullptr;

          public:
            template <typename TYPE>
            BaseObject(CComPtr<TYPE> &d3dSource)
            {
                if (d3dSource)
                {
                    InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), d3dSource);
                    d3dObject->AddRef();
                }
            }

            virtual ~BaseObject(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<TYPE *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "object";
            }
        };

        template <typename TYPE, typename BASE = Render::Object>
        class BaseVideoObject
            : public BASE
        {
          public:
            TYPE *d3dObject = nullptr;

          public:
            template <typename SOURCE>
            BaseVideoObject(CComPtr<SOURCE> &d3dSource)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), dynamic_cast<TYPE *>(d3dSource.p));
                d3dObject->AddRef();
            }

            virtual ~BaseVideoObject(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<TYPE *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "video_object";
            }
        };

        template <typename TYPE, typename BASE>
        class DescribedVideoObject
            : public BASE
        {
          public:
            TYPE *d3dObject = nullptr;
            typename BASE::Description description;

          public:
            template <typename SOURCE>
            DescribedVideoObject(CComPtr<SOURCE> &d3dSource, typename BASE::Description const &description)
                : description(description)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), dynamic_cast<TYPE *>(d3dSource.p));
                d3dObject->AddRef();
            }

            virtual ~DescribedVideoObject(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<TYPE *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            typename BASE::Description const &getDescription(void) const
            {
                return description;
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }
        };

        using CommandList = BaseVideoObject<ID3D11CommandList>;
        using RenderState = DescribedVideoObject<ID3D11RasterizerState, Render::RenderState>;
        using DepthState = DescribedVideoObject<ID3D11DepthStencilState, Render::DepthState>;
        using BlendState = DescribedVideoObject<ID3D11BlendState, Render::BlendState>;
        using SamplerState = DescribedVideoObject<ID3D11SamplerState, Render::SamplerState>;
        using InputLayout = BaseVideoObject<ID3D11InputLayout>;

        using Resource = BaseObject<ID3D11Resource>;
        using ShaderResourceView = BaseObject<ID3D11ShaderResourceView>;
        using UnorderedAccessView = BaseObject<ID3D11UnorderedAccessView>;
        using RenderTargetView = BaseObject<ID3D11RenderTargetView>;

        class Query
            : public Render::Query
        {
          public:
            ID3D11Query *d3dObject = nullptr;

          public:
            Query(CComPtr<ID3D11Query> &d3dSource)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), d3dSource.p);
                d3dObject->AddRef();
            }

            virtual ~Query(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<ID3D11Query *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return "query";
            }
        };

        class Buffer
            : public Render::Buffer,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            ID3D11Buffer *d3dObject = nullptr;
            Render::Buffer::Description description;

          public:
            Buffer(CComPtr<ID3D11Buffer> &d3dBuffer,
                   CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                   CComPtr<ID3D11UnorderedAccessView> &d3dUnorderedAccessView,
                   const Render::Buffer::Description &description)
                : Resource(d3dBuffer), ShaderResourceView(d3dShaderResourceView), UnorderedAccessView(d3dUnorderedAccessView), description(description)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), d3dBuffer);
                d3dObject->AddRef();
            }

            virtual ~Buffer(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<ID3D11Buffer *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Buffer
            const Render::Buffer::Description &getDescription(void) const
            {
                return description;
            }
        };

        class BaseTexture
        {
          public:
            Render::Texture::Description description;

          public:
            BaseTexture(const Render::Texture::Description &description)
                : description(description)
            {
            }
        };

        class Texture
            : virtual public Render::Texture,
              public BaseTexture
        {
          public:
            Texture(const Render::Texture::Description &description)
                : BaseTexture(description)
            {
            }

            virtual ~Texture(void) = default;

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Texture
            const Render::Texture::Description &getDescription(void) const
            {
                return description;
            }
        };

        class ViewTexture
            : public Texture,
              public Resource,
              public ShaderResourceView
        {
          public:
            ViewTexture(CComPtr<ID3D11Resource> &d3dResource,
                        CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                        const Render::Texture::Description &description)
                : Texture(description), Resource(d3dResource), ShaderResourceView(d3dShaderResourceView)
            {
            }
        };

        class UnorderedViewTexture
            : public Texture,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            UnorderedViewTexture(CComPtr<ID3D11Resource> &d3dResource,
                                 CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                                 CComPtr<ID3D11UnorderedAccessView> &d3dUnorderedAccessView,
                                 const Render::Texture::Description &description)
                : Texture(description), Resource(d3dResource), ShaderResourceView(d3dShaderResourceView), UnorderedAccessView(d3dUnorderedAccessView)
            {
            }
        };

        class Target
            : virtual public Render::Target,
              public BaseTexture
        {
          public:
            Render::ViewPort viewPort;

          public:
            Target(const Render::Texture::Description &description)
                : BaseTexture(description), viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(description.width), float(description.height)), 0.0f, 1.0f)
            {
            }

            virtual ~Target(void) = default;

            // Render::Object
            std::string_view getName(void) const
            {
                return description.name;
            }

            // Render::Texture
            const Render::Texture::Description &getDescription(void) const
            {
                return description;
            }

            // Render::Target
            const Render::ViewPort &getViewPort(void) const
            {
                return viewPort;
            }
        };

        class TargetTexture
            : public Target,
              public Resource,
              public RenderTargetView
        {
          public:
            TargetTexture(CComPtr<ID3D11Resource> &d3dResource,
                          CComPtr<ID3D11RenderTargetView> &d3dRenderTargetView,
                          const Render::Texture::Description &description)
                : Target(description), Resource(d3dResource), RenderTargetView(d3dRenderTargetView)
            {
            }

            virtual ~TargetTexture(void) = default;
        };

        class TargetViewTexture
            : virtual public TargetTexture,
              public ShaderResourceView
        {
          public:
            TargetViewTexture(CComPtr<ID3D11Resource> &d3dResource,
                              CComPtr<ID3D11RenderTargetView> &d3dRenderTargetView,
                              CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                              const Render::Texture::Description &description)
                : TargetTexture(d3dResource, d3dRenderTargetView, description), ShaderResourceView(d3dShaderResourceView)
            {
            }

            virtual ~TargetViewTexture(void) = default;
        };

        class UnorderedTargetViewTexture
            : virtual public TargetTexture,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            UnorderedTargetViewTexture(CComPtr<ID3D11Resource> &d3dResource,
                                       CComPtr<ID3D11RenderTargetView> &d3dRenderTargetView,
                                       CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                                       CComPtr<ID3D11UnorderedAccessView> &d3dUnorderedAccessView,
                                       const Render::Texture::Description &description)
                : TargetTexture(d3dResource, d3dRenderTargetView, description), ShaderResourceView(d3dShaderResourceView), UnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            virtual ~UnorderedTargetViewTexture(void) = default;
        };

        class DepthTexture
            : public Texture,
              public Resource,
              public ShaderResourceView,
              public UnorderedAccessView
        {
          public:
            ID3D11DepthStencilView *d3dObject = nullptr;

          public:
            DepthTexture(CComPtr<ID3D11Resource> &d3dResource,
                         CComPtr<ID3D11DepthStencilView> &d3dDepthStencilView,
                         CComPtr<ID3D11ShaderResourceView> &d3dShaderResourceView,
                         CComPtr<ID3D11UnorderedAccessView> &d3dUnorderedAccessView,
                         const Render::Texture::Description &description)
                : Texture(description), Resource(d3dResource), ShaderResourceView(d3dShaderResourceView), UnorderedAccessView(d3dUnorderedAccessView)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), d3dDepthStencilView.p);
                d3dObject->AddRef();
            }

            virtual ~DepthTexture(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<ID3D11DepthStencilView *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }
        };

        template <typename D3DTYPE>
        class Program
            : public Render::Program
        {
          public:
            Render::Program::Information information;
            D3DTYPE *d3dObject = nullptr;

          public:
            template <typename SOURCE>
            Program(CComPtr<SOURCE> &d3dSource, Render::Program::Information information)
                : information(information)
            {
                InterlockedExchangePointer(reinterpret_cast<void **>(&d3dObject), dynamic_cast<D3DTYPE *>(d3dSource.p));
                d3dObject->AddRef();
            }

            virtual ~Program(void)
            {
                if (d3dObject)
                {
                    reinterpret_cast<D3DTYPE *>(InterlockedExchangePointer((void **)&d3dObject, nullptr))->Release();
                }
            }

            // Render::Object
            std::string_view getName(void) const
            {
                return information.name;
            }

            // Render::Program
            Information const &getInformation(void) const
            {
                return information;
            }
        };

        using ComputeProgram = Program<ID3D11ComputeShader>;
        using VertexProgram = Program<ID3D11VertexShader>;
        using GeometryProgram = Program<ID3D11GeometryShader>;
        using PixelProgram = Program<ID3D11PixelShader>;

        GEK_CONTEXT_USER(Device, Window::Device *, Render::Device::Description)
        , public Render::Debug::Device
        {
            class Context
                : public Render::Device::Context
            {
                class ComputePipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                  public:
                    ComputePipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        assert(d3dDeviceContext);
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Compute;
                    }

                    void setProgram(Render::Program *program)
                    {
                        assert(d3dDeviceContext);

                        d3dDeviceContext->CSSetShader(getObject<ComputeProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->CSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->CSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->CSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    ObjectCache<ID3D11UnorderedAccessView> unorderedAccessCache;
                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(d3dDeviceContext);

                        unorderedAccessCache.set<UnorderedAccessView>(list);
                        d3dDeviceContext->CSSetUnorderedAccessViews(firstStage, UINT(list.size()), unorderedAccessCache.get(), countList);
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->CSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->CSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->CSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        unorderedAccessCache.clear(count);
                        d3dDeviceContext->CSSetUnorderedAccessViews(firstStage, count, unorderedAccessCache.get(), nullptr);
                    }
                };

                class VertexPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                  public:
                    VertexPipeline(Context *context)
                        : context(context), d3dDeviceContext(context->d3dDeviceContext)
                    {
                        assert(d3dDeviceContext);
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Vertex;
                    }

                    void setProgram(Render::Program *program)
                    {
                        assert(d3dDeviceContext);

                        if (auto *vertexProgram = dynamic_cast<Gek::Render::Implementation::VertexProgram *>(program))
                        {
                            context->currentVertexProgramName = std::string(vertexProgram->getInformation().name);
                        }
                        else
                        {
                            context->currentVertexProgramName.clear();
                        }

                        d3dDeviceContext->VSSetShader(getObject<VertexProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->VSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->VSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->VSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(false && "Vertex pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->VSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->VSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->VSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(false && "Vertex pipeline does not supported unordered access");
                    }
                };

                class GeometryPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                  public:
                    GeometryPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        assert(d3dDeviceContext);
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Geometry;
                    }

                    void setProgram(Render::Program *program)
                    {
                        assert(d3dDeviceContext);

                        d3dDeviceContext->GSSetShader(getObject<GeometryProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->GSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->GSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->GSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(false && "Geometry pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->GSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->GSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->GSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(false && "Geometry pipeline does not supported unordered access");
                    }
                };

                class PixelPipeline
                    : public Render::Device::Context::Pipeline
                {
                  private:
                    Context *context = nullptr;
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                  public:
                    PixelPipeline(Context *context)
                        : context(context), d3dDeviceContext(context->d3dDeviceContext)
                    {
                        assert(d3dDeviceContext);
                    }

                    // Render::Pipeline
                    Type getType(void) const
                    {
                        return Type::Pixel;
                    }

                    void setProgram(Render::Program *program)
                    {
                        assert(d3dDeviceContext);

                        if (auto *pixelProgram = dynamic_cast<Gek::Render::Implementation::PixelProgram *>(program))
                        {
                            context->currentPixelProgramName = std::string(pixelProgram->getInformation().name);
                        }
                        else
                        {
                            context->currentPixelProgramName.clear();
                        }

                        d3dDeviceContext->PSSetShader(getObject<PixelProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->PSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Render::Buffer *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->PSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Render::Object *> &list, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->PSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    ObjectCache<ID3D11UnorderedAccessView> unorderedAccessCache;
                    void setUnorderedAccessList(const std::vector<Render::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        assert(d3dDeviceContext);

                        unorderedAccessCache.set<UnorderedAccessView>(list);
                        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, UINT(list.size()), unorderedAccessCache.get(), countList);
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->PSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->PSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->PSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        assert(d3dDeviceContext);

                        unorderedAccessCache.clear(count);
                        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, count, unorderedAccessCache.get(), nullptr);
                    }
                };

              public:
                Device *pipelineDevice = nullptr;
                CComPtr<ID3D11DeviceContext> d3dDeviceContext;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;
                std::string currentVertexProgramName;
                std::string currentPixelProgramName;
                Render::RenderState::CullMode currentCullMode = Render::RenderState::CullMode::None;
                bool currentFrontCounterClockwise = false;
                bool hasCurrentRenderState = false;
                bool currentDepthEnable = false;
                bool currentDepthWrite = false;
                Render::ComparisonFunction currentDepthCompare = Render::ComparisonFunction::Always;
                bool hasCurrentDepthState = false;

              public:
                Context(Device *pipelineDevice, CComPtr<ID3D11DeviceContext> &d3dDeviceContext)
                    : pipelineDevice(pipelineDevice), d3dDeviceContext(d3dDeviceContext), computeSystemHandler(new ComputePipeline(d3dDeviceContext)), vertexSystemHandler(new VertexPipeline(this)), geomtrySystemHandler(new GeometryPipeline(d3dDeviceContext)), pixelSystemHandler(new PixelPipeline(this))
                {
                    assert(d3dDeviceContext);
                    assert(computeSystemHandler);
                    assert(vertexSystemHandler);
                    assert(geomtrySystemHandler);
                    assert(pixelSystemHandler);
                }

                // Render::Context
                Pipeline *const computePipeline(void)
                {
                    assert(computeSystemHandler);

                    return computeSystemHandler.get();
                }

                Pipeline *const vertexPipeline(void)
                {
                    assert(vertexSystemHandler);

                    return vertexSystemHandler.get();
                }

                Pipeline *const geometryPipeline(void)
                {
                    assert(geomtrySystemHandler);

                    return geomtrySystemHandler.get();
                }

                Pipeline *const pixelPipeline(void)
                {
                    assert(pixelSystemHandler);

                    return pixelSystemHandler.get();
                }

                void begin(Render::Query *query)
                {
                    assert(d3dDeviceContext);
                    assert(query);

                    d3dDeviceContext->Begin(getObject<Query>(query));
                }

                void end(Render::Query *query)
                {
                    assert(d3dDeviceContext);
                    assert(query);

                    d3dDeviceContext->End(getObject<Query>(query));
                }

                Render::Query::Status getData(Render::Query *query, void *data, size_t dataSize, bool waitUntilReady = false)
                {
                    assert(d3dDeviceContext);
                    assert(query);

                    auto queryObject = getObject<Query>(query);
                    if (waitUntilReady)
                    {
                        while (d3dDeviceContext->GetData(queryObject, nullptr, 0, 0) == S_FALSE)
                        {
                            Sleep(1);
                        };
                    }

                    switch (d3dDeviceContext->GetData(queryObject, data, UINT(dataSize), 0))
                    {
                    case S_OK:
                        return Render::Query::Status::Ready;

                    case S_FALSE:
                        return Render::Query::Status::Waiting;
                    };

                    return Render::Query::Status::Error;
                }

                void generateMipMaps(Render::Texture *texture)
                {
                    assert(d3dDeviceContext);
                    assert(texture);

                    d3dDeviceContext->GenerateMips(getObject<ShaderResourceView>(texture));
                }

                void resolveSamples(Render::Texture *destination, Render::Texture *source)
                {
                    assert(d3dDeviceContext);
                    assert(destination);
                    assert(source);

                    d3dDeviceContext->ResolveSubresource(getObject<Resource>(destination), 0, getObject<Resource>(source), 0, Render::Implementation::TextureFormatList[static_cast<uint8_t>(destination->getDescription().format)]);
                }

                void copyResource(Render::Object *destination, Render::Object *source)
                {
                    assert(d3dDeviceContext);
                    assert(destination);
                    assert(source);

                    auto destinationTexture = dynamic_cast<BaseTexture *>(destination);
                    auto sourceTexture = dynamic_cast<BaseTexture *>(source);
                    if (destinationTexture && sourceTexture)
                    {
                        if (destinationTexture->description.width != sourceTexture->description.width ||
                            destinationTexture->description.height != sourceTexture->description.height ||
                            destinationTexture->description.depth != sourceTexture->description.depth)
                        {
                            return;
                        }

                        if (destinationTexture->description.mipMapCount > 0 || sourceTexture->description.mipMapCount > 0)
                        {
                            d3dDeviceContext->CopySubresourceRegion(getObject<Resource>(destination), 0, 0, 0, 0, getObject<Resource>(source), 0, nullptr);
                            return;
                        }
                    }

                    d3dDeviceContext->CopyResource(getObject<Resource>(destination), getObject<Resource>(source));
                }

                void clearState(void)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->ClearState();
                }

                void setViewportList(const std::vector<Render::ViewPort> &viewPortList)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->RSSetViewports(UINT(viewPortList.size()), (D3D11_VIEWPORT *)viewPortList.data());
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->RSSetScissorRects(UINT(rectangleList.size()), (D3D11_RECT *)rectangleList.data());
                }

                void clearResource(Render::Object *object, Math::Float4 const &value)
                {
                    assert(d3dDeviceContext);
                    assert(object);
                }

                void clearUnorderedAccess(Render::Object *object, Math::Float4 const &value)
                {
                    assert(d3dDeviceContext);
                    assert(object);

                    d3dDeviceContext->ClearUnorderedAccessViewFloat(getObject<UnorderedAccessView>(object), value.data);
                }

                void clearUnorderedAccess(Render::Object *object, Math::UInt4 const &value)
                {
                    assert(d3dDeviceContext);
                    assert(object);

                    d3dDeviceContext->ClearUnorderedAccessViewUint(getObject<UnorderedAccessView>(object), value.data);
                }

                void clearRenderTarget(Render::Target *renderTarget, Math::Float4 const &clearColor)
                {
                    assert(d3dDeviceContext);
                    assert(renderTarget);

                    d3dDeviceContext->ClearRenderTargetView(getObject<RenderTargetView>(renderTarget), clearColor.data);
                }

                void clearDepthStencilTarget(Render::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    assert(d3dDeviceContext);
                    assert(depthBuffer);

                    d3dDeviceContext->ClearDepthStencilView(getObject<DepthTexture>(depthBuffer),
                                                            ((flags & Render::ClearFlags::Depth ? D3D11_CLEAR_DEPTH : 0) |
                                                             (flags & Render::ClearFlags::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                                                            clearDepth, clearStencil);
                }

                void clearIndexBuffer(void)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
                }

                void clearVertexBufferList(uint32_t count, uint32_t firstSlot)
                {
                    assert(d3dDeviceContext);

                    vertexBufferCache.clear(count);
                    vertexBufferStrideCache.resize(count);
                    vertexBufferOffsetsCache.resize(count);
                    d3dDeviceContext->IASetVertexBuffers(firstSlot, count, vertexBufferCache.get(), vertexBufferStrideCache.data(), vertexBufferOffsetsCache.data());
                }

                void clearRenderTargetList(uint32_t count, bool depthBuffer)
                {
                    assert(d3dDeviceContext);

                    renderTargetViewCache.clear(count);
                    d3dDeviceContext->OMSetRenderTargets(count, renderTargetViewCache.get(), nullptr);
                }

                ObjectCache<ID3D11RenderTargetView> renderTargetViewCache;
                void setRenderTargetList(const std::vector<Render::Target *> &renderTargetList, Render::Object *depthBuffer)
                {
                    assert(d3dDeviceContext);

                    renderTargetViewCache.set<RenderTargetView>(renderTargetList);
                    d3dDeviceContext->OMSetRenderTargets(UINT(renderTargetList.size()), renderTargetViewCache.get(), getObject<DepthTexture>(depthBuffer));
                }

                void setRenderState(Render::Object *renderState)
                {
                    assert(d3dDeviceContext);
                    assert(renderState);

                    if (auto *d3dRenderState = dynamic_cast<Gek::Render::Implementation::RenderState *>(renderState))
                    {
                        const auto &description = d3dRenderState->getDescription();
                        currentCullMode = description.cullMode;
                        currentFrontCounterClockwise = description.frontCounterClockwise;
                        hasCurrentRenderState = true;
                    }
                    else
                    {
                        hasCurrentRenderState = false;
                    }

                    d3dDeviceContext->RSSetState(getObject<RenderState>(renderState));
                }

                void setDepthState(Render::Object *depthState, uint32_t stencilReference)
                {
                    assert(d3dDeviceContext);
                    assert(depthState);

                    if (auto *d3dDepthState = dynamic_cast<Gek::Render::Implementation::DepthState *>(depthState))
                    {
                        const auto &description = d3dDepthState->getDescription();
                        currentDepthEnable = description.enable;
                        currentDepthWrite = (description.writeMask != Render::DepthState::Write::Zero);
                        currentDepthCompare = description.comparisonFunction;
                        hasCurrentDepthState = true;
                    }
                    else
                    {
                        hasCurrentDepthState = false;
                    }

                    d3dDeviceContext->OMSetDepthStencilState(getObject<DepthState>(depthState), stencilReference);
                }

                void setBlendState(Render::Object *blendState, Math::Float4 const &blendFactor, uint32_t mask)
                {
                    assert(d3dDeviceContext);
                    assert(blendState);

                    d3dDeviceContext->OMSetBlendState(getObject<BlendState>(blendState), blendFactor.data, mask);
                }

                void setInputLayout(Render::Object *inputLayout)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->IASetInputLayout(getObject<InputLayout>(inputLayout));
                }

                void setIndexBuffer(Render::Buffer *indexBuffer, uint32_t offset)
                {
                    assert(d3dDeviceContext);
                    assert(indexBuffer);

                    DXGI_FORMAT format = Render::Implementation::BufferFormatList[static_cast<uint8_t>(indexBuffer->getDescription().format)];
                    d3dDeviceContext->IASetIndexBuffer(getObject<Buffer>(indexBuffer), format, offset);
                }

                ObjectCache<ID3D11Buffer> vertexBufferCache;
                std::vector<uint32_t> vertexBufferStrideCache;
                std::vector<uint32_t> vertexBufferOffsetsCache;
                void setVertexBufferList(const std::vector<Render::Buffer *> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                    assert(d3dDeviceContext);

                    uint32_t vertexBufferCount = UINT(vertexBufferList.size());
                    vertexBufferStrideCache.resize(vertexBufferCount);
                    vertexBufferOffsetsCache.resize(vertexBufferCount);
                    for (uint32_t buffer = 0; buffer < vertexBufferCount; ++buffer)
                    {
                        vertexBufferStrideCache[buffer] = vertexBufferList[buffer]->getDescription().stride;
                        vertexBufferOffsetsCache[buffer] = (offsetList ? offsetList[buffer] : 0);
                    }

                    vertexBufferCache.set<Buffer>(vertexBufferList);
                    d3dDeviceContext->IASetVertexBuffers(firstSlot, vertexBufferCount, vertexBufferCache.get(), vertexBufferStrideCache.data(), vertexBufferOffsetsCache.data());
                }

                void setPrimitiveType(Render::PrimitiveType primitiveType)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->IASetPrimitiveTopology(Render::Implementation::TopologList[static_cast<uint8_t>(primitiveType)]);
                }

                void drawPrimitive(uint32_t vertexCount, uint32_t firstVertex)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->Draw(vertexCount, firstVertex);
                }

                void drawInstancedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t vertexCount, uint32_t firstVertex)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
                }

                void drawIndexedPrimitive(uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    assert(d3dDeviceContext);

                    if (pipelineDevice && !pipelineDevice->loggedFirstIndexedStateThisFrame)
                    {
                        const auto engineCullMode = hasCurrentRenderState ? currentCullMode : Render::RenderState::CullMode::None;
                        const bool engineFrontCCW = hasCurrentRenderState ? currentFrontCounterClockwise : false;
                        const bool engineDepthEnable = hasCurrentDepthState ? currentDepthEnable : false;
                        const bool engineDepthWrite = hasCurrentDepthState ? currentDepthWrite : false;
                        const auto engineDepthCompare = hasCurrentDepthState ? currentDepthCompare : Render::ComparisonFunction::Always;
                        const auto mappedCullMode = Render::Implementation::CullModeList[static_cast<uint8_t>(engineCullMode)];
                        const auto mappedDepthCompare = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(engineDepthCompare)];

                        pipelineDevice->loggedFirstIndexedStateThisFrame = true;
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.frame", static_cast<double>(pipelineDevice->sampleFrameIndex));
                        pipelineDevice->getContext()->setRuntimeMetric("render.frame", static_cast<double>(pipelineDevice->sampleFrameIndex));
                        pipelineDevice->getContext()->setRuntimeMetric("render.backend", 1.0);
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineCull", static_cast<double>(engineCullMode));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineFrontCCW", static_cast<double>(engineFrontCCW ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthEnable", static_cast<double>(engineDepthEnable ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthWrite", static_cast<double>(engineDepthWrite ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthCompare", static_cast<double>(engineDepthCompare));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.mappedCull", static_cast<double>(mappedCullMode));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.mappedDepthFunc", static_cast<double>(mappedDepthCompare));
                        pipelineDevice->getContext()->setRuntimeMetric("render.mappedDepthFunc", static_cast<double>(mappedDepthCompare));
                    }

                    d3dDeviceContext->DrawIndexed(indexCount, firstIndex, firstVertex);
                }

                void drawInstancedIndexedPrimitive(uint32_t instanceCount, uint32_t firstInstance, uint32_t indexCount, uint32_t firstIndex, uint32_t firstVertex)
                {
                    assert(d3dDeviceContext);

                    if (pipelineDevice && !pipelineDevice->loggedFirstIndexedStateThisFrame)
                    {
                        const auto engineCullMode = hasCurrentRenderState ? currentCullMode : Render::RenderState::CullMode::None;
                        const bool engineFrontCCW = hasCurrentRenderState ? currentFrontCounterClockwise : false;
                        const bool engineDepthEnable = hasCurrentDepthState ? currentDepthEnable : false;
                        const bool engineDepthWrite = hasCurrentDepthState ? currentDepthWrite : false;
                        const auto engineDepthCompare = hasCurrentDepthState ? currentDepthCompare : Render::ComparisonFunction::Always;
                        const auto mappedCullMode = Render::Implementation::CullModeList[static_cast<uint8_t>(engineCullMode)];
                        const auto mappedDepthCompare = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(engineDepthCompare)];

                        pipelineDevice->loggedFirstIndexedStateThisFrame = true;
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.frame", static_cast<double>(pipelineDevice->sampleFrameIndex));
                        pipelineDevice->getContext()->setRuntimeMetric("render.frame", static_cast<double>(pipelineDevice->sampleFrameIndex));
                        pipelineDevice->getContext()->setRuntimeMetric("render.backend", 1.0);
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineCull", static_cast<double>(engineCullMode));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineFrontCCW", static_cast<double>(engineFrontCCW ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthEnable", static_cast<double>(engineDepthEnable ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthWrite", static_cast<double>(engineDepthWrite ? 1u : 0u));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.engineDepthCompare", static_cast<double>(engineDepthCompare));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.mappedCull", static_cast<double>(mappedCullMode));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.mappedDepthFunc", static_cast<double>(mappedDepthCompare));
                        pipelineDevice->getContext()->setRuntimeMetric("d3d11.firstIndexedInstanceCount", static_cast<double>(instanceCount));
                        pipelineDevice->getContext()->setRuntimeMetric("render.mappedDepthFunc", static_cast<double>(mappedDepthCompare));
                        pipelineDevice->getContext()->setRuntimeMetric("render.firstIndexedInstanceCount", static_cast<double>(instanceCount));
                    }

                    d3dDeviceContext->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
                }

                void dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
                {
                    assert(d3dDeviceContext);

                    d3dDeviceContext->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
                }

                Render::ObjectPtr finishCommandList(void)
                {
                    assert(d3dDeviceContext);

                    CComPtr<ID3D11CommandList> d3dCommandList;
                    HRESULT resultValue = d3dDeviceContext->FinishCommandList(FALSE, &d3dCommandList);
                    if (FAILED(resultValue) || !d3dCommandList)
                    {
                        pipelineDevice->getContext()->log(Gek::Context::Error, "Unable to finish command list compilation");
                        return nullptr;
                    }

                    return std::make_unique<CommandList>(d3dCommandList);
                }
            };

          public:
            Window::Device *window = nullptr;
            bool isChildWindow = false;

            CComPtr<ID3D11Device> d3dDevice;
            CComPtr<ID3D11DeviceContext> d3dDeviceContext;
            CComPtr<IDXGISwapChain1> dxgiSwapChain;

            Render::Device::ContextPtr defaultContext;
            Render::TargetPtr backBuffer;
            uint64_t sampleFrameIndex = 0;
            bool loggedFirstIndexedStateThisFrame = false;

          public:
            Device(Gek::Context * context, Window::Device * window, Render::Device::Description deviceDescription)
                : ContextRegistration(context), window(window), isChildWindow(GetParent(reinterpret_cast<HWND>(window->getWindowData(0))) != nullptr)
            {
                assert(window);

                {
                    const auto markerPath = getContext()->getCachePath("renderer_runtime_marker.txt").getString();
                    std::ofstream runtimeMarker(markerPath, std::ios::out | std::ios::trunc);
                    if (runtimeMarker.is_open())
                    {
                        runtimeMarker << "backend=d3d11" << std::endl;
                    }
                }

                UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
                flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

                D3D_FEATURE_LEVEL featureLevelList[] = {
                    D3D_FEATURE_LEVEL_11_0,
                };

                D3D_FEATURE_LEVEL featureLevel;
                HRESULT resultValue = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, featureLevelList, 1, D3D11_SDK_VERSION, &d3dDevice, &featureLevel, &d3dDeviceContext);
                if (featureLevel != featureLevelList[0])
                {
                    throw std::runtime_error("Direct3D 11.0 feature level required");
                }

                if (FAILED(resultValue) || !d3dDevice || !d3dDeviceContext)
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
                swapChainDescription.Format = Render::Implementation::TextureFormatList[static_cast<uint8_t>(deviceDescription.displayFormat)];
                swapChainDescription.Stereo = false;
                swapChainDescription.SampleDesc.Count = deviceDescription.sampleCount;
                swapChainDescription.SampleDesc.Quality = deviceDescription.sampleQuality;
                swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
                swapChainDescription.BufferCount = 2;
                swapChainDescription.Scaling = DXGI_SCALING_STRETCH;
                swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
                swapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
                swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
                resultValue = dxgiFactory->CreateSwapChainForHwnd(d3dDevice, reinterpret_cast<HWND>(window->getWindowData(0)), &swapChainDescription, nullptr, nullptr, &dxgiSwapChain);
                if (FAILED(resultValue) || !dxgiSwapChain)
                {
                    throw std::runtime_error("Unable to create swap chain for window");
                }

                dxgiFactory->MakeWindowAssociation(reinterpret_cast<HWND>(window->getWindowData(0)), 0);

#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                CComQIPtr<ID3D11InfoQueue> d3dInfoQueue(d3dDebug);
                // d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                // d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                // d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif

                defaultContext = std::make_unique<Context>(this, d3dDeviceContext);
            }

            ~Device(void)
            {
                setFullScreenState(false);

                backBuffer = nullptr;
                defaultContext = nullptr;

                dxgiSwapChain.Release();
                d3dDeviceContext.Release();

                d3dDevice.Release();
            }

            // Render::Debug::Device
            void *getDevice(void)
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
                    dxgiOutput->GetDisplayModeList(Render::Implementation::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, nullptr);

                    std::vector<DXGI_MODE_DESC> dxgiDisplayModeList(modeCount);
                    dxgiOutput->GetDisplayModeList(Render::Implementation::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, dxgiDisplayModeList.data());
                    for (auto const &dxgiDisplayMode : dxgiDisplayModeList)
                    {
                        Render::DisplayMode displayMode(dxgiDisplayMode.Width, dxgiDisplayMode.Height, Render::Implementation::GetFormat(dxgiDisplayMode.Format));
                        displayMode.refreshRate.numerator = dxgiDisplayMode.RefreshRate.Numerator;
                        displayMode.refreshRate.denominator = dxgiDisplayMode.RefreshRate.Denominator;
                        if (![&](void) -> bool
                            {
                                for (auto &checkMode : displayModeList)
                                {
                                    if (memcmp(&checkMode, &displayMode, sizeof(Render::DisplayMode)) == 0)
                                    {
                                        return true;
                                    }
                                }

                                return false;
                            }())
                        {
                            displayModeList.push_back(displayMode);
                        }
                    }

                    std::sort(std::execution::par, std::begin(displayModeList), std::end(displayModeList), [](const Render::DisplayMode &left, const Render::DisplayMode &right) -> bool
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

                        return false; });
                }

                return displayModeList;
            }

            void setFullScreenState(bool fullScreen)
            {
                assert(d3dDeviceContext);
                assert(dxgiSwapChain);

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;

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

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;

                DXGI_MODE_DESC description;
                description.Width = displayMode.width;
                description.Height = displayMode.height;
                description.RefreshRate.Numerator = displayMode.refreshRate.numerator;
                description.RefreshRate.Denominator = displayMode.refreshRate.denominator;
                description.Format = Render::Implementation::TextureFormatList[static_cast<uint8_t>(displayMode.format)];
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

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;

                HRESULT resultValue = dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
                if (FAILED(resultValue))
                {
                    throw std::runtime_error("Unable to resize swap chain buffers to window size");
                }
            }

            std::mutex backBufferMutex;
            Render::Target *const getBackBuffer(void)
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
                        description.format = Render::Implementation::GetFormat(textureDescription.Format);
                        auto d3dRenderTargetResource = CComQIPtr<ID3D11Resource>(d3dRenderTarget);
                        backBuffer = std::make_unique<TargetViewTexture>(d3dRenderTargetResource, d3dRenderTargetView, d3dShaderResourceView, description);
                    }
                }

                return backBuffer.get();
            }

            Render::Device::Context *const getDefaultContext(void)
            {
                assert(defaultContext);

                return defaultContext.get();
            }

            Render::Device::ContextPtr createDeferredContext(void)
            {
                assert(d3dDevice);

                CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
                HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext);
                if (FAILED(resultValue) || !d3dDeferredDeviceContext)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create deferred context");
                    return nullptr;
                }

                return std::make_unique<Context>(this, d3dDeferredDeviceContext);
            }

            Render::QueryPtr createQuery(Render::Query::Type type)
            {
                assert(d3dDevice);

                D3D11_QUERY_DESC description;
                description.Query = Render::Implementation::QueryList[static_cast<uint8_t>(type)];
                description.MiscFlags = 0;

                CComPtr<ID3D11Query> d3dQuery;
                HRESULT resultValue = d3dDevice->CreateQuery(&description, &d3dQuery);
                if (FAILED(resultValue) || !d3dQuery)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create event");
                    return nullptr;
                }

                return std::make_unique<Query>(d3dQuery);
            }

            Render::RenderStatePtr createRenderState(Render::RenderState::Description const &description)
            {
                assert(d3dDevice);

                D3D11_RASTERIZER_DESC rasterizerDescription;
                rasterizerDescription.FrontCounterClockwise = description.frontCounterClockwise;
                rasterizerDescription.DepthBias = description.depthBias;
                rasterizerDescription.DepthBiasClamp = description.depthBiasClamp;
                rasterizerDescription.SlopeScaledDepthBias = description.slopeScaledDepthBias;
                rasterizerDescription.DepthClipEnable = description.depthClipEnable;
                rasterizerDescription.ScissorEnable = description.scissorEnable;
                rasterizerDescription.MultisampleEnable = description.multisampleEnable;
                rasterizerDescription.AntialiasedLineEnable = description.antialiasedLineEnable;
                rasterizerDescription.FillMode = Render::Implementation::FillModeList[static_cast<uint8_t>(description.fillMode)];
                rasterizerDescription.CullMode = Render::Implementation::CullModeList[static_cast<uint8_t>(description.cullMode)];

                CComPtr<ID3D11RasterizerState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create rasterizer state");
                    return nullptr;
                }

                return std::make_unique<RenderState>(d3dStates, description);
            }

            Render::DepthStatePtr createDepthState(Render::DepthState::Description const &description)
            {
                assert(d3dDevice);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = description.enable;
                depthStencilDescription.DepthFunc = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(description.comparisonFunction)];
                depthStencilDescription.StencilEnable = description.stencilEnable;
                depthStencilDescription.StencilReadMask = description.stencilReadMask;
                depthStencilDescription.StencilWriteMask = description.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilFrontState.failOperation)];
                depthStencilDescription.FrontFace.StencilDepthFailOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilFrontState.depthFailOperation)];
                depthStencilDescription.FrontFace.StencilPassOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilFrontState.passOperation)];
                depthStencilDescription.FrontFace.StencilFunc = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(description.stencilFrontState.comparisonFunction)];
                depthStencilDescription.BackFace.StencilFailOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilBackState.failOperation)];
                depthStencilDescription.BackFace.StencilDepthFailOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilBackState.depthFailOperation)];
                depthStencilDescription.BackFace.StencilPassOp = Render::Implementation::StencilOperationList[static_cast<uint8_t>(description.stencilBackState.passOperation)];
                depthStencilDescription.BackFace.StencilFunc = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(description.stencilBackState.comparisonFunction)];
                depthStencilDescription.DepthWriteMask = Render::Implementation::DepthWriteMaskList[static_cast<uint8_t>(description.writeMask)];

                CComPtr<ID3D11DepthStencilState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create depth stencil state");
                    return nullptr;
                }

                return std::make_unique<DepthState>(d3dStates, description);
            }

            Render::BlendStatePtr createBlendState(Render::BlendState::Description const &description)
            {
                assert(d3dDevice);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = description.alphaToCoverage;
                blendDescription.IndependentBlendEnable = description.independentBlendStates;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    blendDescription.RenderTarget[renderTarget].BlendEnable = description.targetStates[renderTarget].enable;
                    blendDescription.RenderTarget[renderTarget].SrcBlend = Render::Implementation::BlendSourceList[static_cast<uint8_t>(description.targetStates[renderTarget].colorSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlend = Render::Implementation::BlendSourceList[static_cast<uint8_t>(description.targetStates[renderTarget].colorDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOp = Render::Implementation::BlendOperationList[static_cast<uint8_t>(description.targetStates[renderTarget].colorOperation)];
                    blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = Render::Implementation::BlendSourceList[static_cast<uint8_t>(description.targetStates[renderTarget].alphaSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlendAlpha = Render::Implementation::BlendSourceList[static_cast<uint8_t>(description.targetStates[renderTarget].alphaDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOpAlpha = Render::Implementation::BlendOperationList[static_cast<uint8_t>(description.targetStates[renderTarget].alphaOperation)];
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                    if (description.targetStates[renderTarget].writeMask & Render::BlendState::Mask::R)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (description.targetStates[renderTarget].writeMask & Render::BlendState::Mask::G)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (description.targetStates[renderTarget].writeMask & Render::BlendState::Mask::B)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (description.targetStates[renderTarget].writeMask & Render::BlendState::Mask::A)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                CComPtr<ID3D11BlendState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create independent blend state");
                    return nullptr;
                }

                return std::make_unique<BlendState>(d3dStates, description);
            }

            Render::SamplerStatePtr createSamplerState(Render::SamplerState::Description const &description)
            {
                assert(d3dDevice);

                D3D11_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = Render::Implementation::AddressModeList[static_cast<uint8_t>(description.addressModeU)];
                samplerDescription.AddressV = Render::Implementation::AddressModeList[static_cast<uint8_t>(description.addressModeV)];
                samplerDescription.AddressW = Render::Implementation::AddressModeList[static_cast<uint8_t>(description.addressModeW)];
                samplerDescription.MipLODBias = description.mipLevelBias;
                samplerDescription.MaxAnisotropy = description.maximumAnisotropy;
                samplerDescription.ComparisonFunc = Render::Implementation::ComparisonFunctionList[static_cast<uint8_t>(description.comparisonFunction)];
                samplerDescription.BorderColor[0] = description.borderColor.r;
                samplerDescription.BorderColor[1] = description.borderColor.g;
                samplerDescription.BorderColor[2] = description.borderColor.b;
                samplerDescription.BorderColor[3] = description.borderColor.a;
                samplerDescription.MinLOD = description.minimumMipLevel;
                samplerDescription.MaxLOD = description.maximumMipLevel;
                samplerDescription.Filter = Render::Implementation::FilterList[static_cast<uint8_t>(description.filterMode)];

                CComPtr<ID3D11SamplerState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create sampler state");
                    return nullptr;
                }

                return std::make_unique<SamplerState>(d3dStates, description);
            }

            Render::BufferPtr createBuffer(const Render::Buffer::Description &description, const void *data)
            {
                assert(d3dDevice);

                if (description.count == 0)
                {
                    return nullptr;
                }

                uint32_t stride = description.stride;
                if (description.format != Render::Format::Unknown)
                {
                    if (description.stride > 0)
                    {
                        getContext()->log(Gek::Context::Error, "Buffer requires only a format or an element stride");
                        return nullptr;
                    }

                    stride = Render::Implementation::FormatStrideList[static_cast<uint8_t>(description.format)];
                }
                else if (description.stride == 0)
                {
                    getContext()->log(Gek::Context::Error, "Buffer requires either a format or an element stride");
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
                        getContext()->log(Gek::Context::Error, "Unable to dynamic buffer");
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
                        getContext()->log(Gek::Context::Error, "Unable to create static buffer");
                        return nullptr;
                    }
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Render::Buffer::Flags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = Render::Implementation::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;
                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        getContext()->log(Gek::Context::Error, "Unable to create buffer shader resource view");
                        return nullptr;
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Render::Buffer::Flags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = Render::Implementation::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;
                    viewDescription.Buffer.Flags = (description.flags & Render::Buffer::Flags::Counter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0);

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        getContext()->log(Gek::Context::Error, "Unable to create buffer unordered access view");
                        return nullptr;
                    }
                }

                return std::make_unique<Buffer>(d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView, description);
            }

            bool mapBuffer(Render::Buffer * buffer, void *&data, Render::Map mapping)
            {
                assert(d3dDeviceContext);

                D3D11_MAP d3dMapping = Render::Implementation::MapList[static_cast<uint8_t>(mapping)];

                D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                mappedSubResource.pData = nullptr;
                mappedSubResource.RowPitch = 0;
                mappedSubResource.DepthPitch = 0;

                if (SUCCEEDED(d3dDeviceContext->Map(getObject<Buffer>(buffer), 0, d3dMapping, 0, &mappedSubResource)))
                {
                    data = mappedSubResource.pData;
                    return true;
                }

                return false;
            }

            void unmapBuffer(Render::Buffer * buffer)
            {
                assert(d3dDeviceContext);
                assert(buffer);

                d3dDeviceContext->Unmap(getObject<Buffer>(buffer), 0);
            }

            void updateResource(Render::Object * object, const void *data)
            {
                assert(d3dDeviceContext);
                assert(object);
                assert(data);

                d3dDeviceContext->UpdateSubresource(getObject<Resource>(object), 0, nullptr, data, 0, 0);
            }

            void copyResource(Render::Object * destination, Render::Object * source)
            {
                assert(d3dDeviceContext);
                assert(destination);
                assert(source);

                auto destinationTexture = dynamic_cast<BaseTexture *>(destination);
                auto sourceTexture = dynamic_cast<BaseTexture *>(source);
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
            }

            std::string_view const getSemanticMoniker(Render::InputElement::Semantic semantic)
            {
                return Render::Implementation::SemanticNameList[static_cast<uint8_t>(semantic)];
            }

            Render::ObjectPtr createInputLayout(const std::vector<Render::InputElement> &elementList, Render::Program::Information const &information)
            {
                if (information.compiledData.empty() ||
                    information.type != Render::Program::Type::Vertex)
                {
                    return nullptr;
                }

                uint32_t semanticIndexList[static_cast<uint8_t>(Render::InputElement::Semantic::Count)] = { 0 };
                std::array<bool, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT> slotHasElement{};
                std::vector<D3D11_INPUT_ELEMENT_DESC> d3dElementList;
                for (auto const &element : elementList)
                {
                    if (element.sourceIndex >= D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT)
                    {
                        getContext()->log(Gek::Context::Error, "Input layout creation failed: sourceIndex {} exceeds D3D11 slot limit.", element.sourceIndex);
                        return nullptr;
                    }

                    D3D11_INPUT_ELEMENT_DESC elementDesc;
                    elementDesc.Format = Render::Implementation::BufferFormatList[static_cast<uint8_t>(element.format)];
                    if (element.alignedByteOffset == Render::InputElement::AppendAligned)
                    {
                        elementDesc.AlignedByteOffset = slotHasElement[element.sourceIndex] ? D3D11_APPEND_ALIGNED_ELEMENT : 0;
                    }
                    else
                    {
                        elementDesc.AlignedByteOffset = element.alignedByteOffset;
                    }
                    elementDesc.SemanticName = Render::Implementation::SemanticNameList[static_cast<uint8_t>(element.semantic)].data();
                    elementDesc.SemanticIndex = semanticIndexList[static_cast<uint8_t>(element.semantic)]++;
                    elementDesc.InputSlot = element.sourceIndex;
                    switch (element.source)
                    {
                    case Render::InputElement::Source::Instance:
                        elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        elementDesc.InstanceDataStepRate = 1;
                        break;

                    case Render::InputElement::Source::Vertex:
                    default:
                        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        elementDesc.InstanceDataStepRate = 0;
                        break;
                    };

                    d3dElementList.push_back(elementDesc);
                    slotHasElement[element.sourceIndex] = true;
                }

                CComPtr<ID3D11InputLayout> d3dInputLayout;
                HRESULT resultValue = d3dDevice->CreateInputLayout(d3dElementList.data(), UINT(d3dElementList.size()), information.compiledData.data(), information.compiledData.size(), &d3dInputLayout);
                if (FAILED(resultValue) || !d3dInputLayout)
                {
                    getContext()->log(Gek::Context::Error, "Unable to create input vertex layout for program '{}' (HRESULT={})", information.name, getHRESULTString(resultValue));

                    for (size_t elementIndex = 0; elementIndex < d3dElementList.size(); ++elementIndex)
                    {
                        auto const &element = d3dElementList[elementIndex];
                        getContext()->log(
                            Gek::Context::Debug,
                            "Layout[{}]: semantic={}{} format={} slot={} offset={} class={} step={}",
                            elementIndex,
                            element.SemanticName,
                            element.SemanticIndex,
                            uint32_t(element.Format),
                            element.InputSlot,
                            element.AlignedByteOffset,
                            (element.InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA ? "instance" : "vertex"),
                            element.InstanceDataStepRate);
                    }

                    CComPtr<ID3D11ShaderReflection> shaderReflection;
                    HRESULT reflectionResult = D3DReflect(
                        information.compiledData.data(),
                        information.compiledData.size(),
                        IID_ID3D11ShaderReflection,
                        reinterpret_cast<void **>(&shaderReflection));

                    if (SUCCEEDED(reflectionResult) && shaderReflection)
                    {
                        D3D11_SHADER_DESC shaderDescription = {};
                        if (SUCCEEDED(shaderReflection->GetDesc(&shaderDescription)))
                        {
                            getContext()->log(Gek::Context::Debug, "Shader input parameter count: {}", shaderDescription.InputParameters);

                            std::vector<bool> usedElement(d3dElementList.size(), false);
                            std::vector<D3D11_INPUT_ELEMENT_DESC> reflectedElementList;
                            reflectedElementList.reserve(shaderDescription.InputParameters);
                            std::vector<std::string> reflectedSemanticNames;
                            reflectedSemanticNames.reserve(shaderDescription.InputParameters);

                            auto splitSemantic = [](std::string_view semanticName, uint32_t semanticIndex) -> std::pair<std::string, uint32_t>
                            {
                                size_t suffixStart = semanticName.size();
                                while (suffixStart > 0 && std::isdigit(static_cast<unsigned char>(semanticName[suffixStart - 1])) != 0)
                                {
                                    --suffixStart;
                                }

                                uint32_t embeddedIndex = 0;
                                if (suffixStart < semanticName.size())
                                {
                                    embeddedIndex = static_cast<uint32_t>(std::strtoul(std::string(semanticName.substr(suffixStart)).c_str(), nullptr, 10));
                                }

                                std::string baseName(semanticName.substr(0, suffixStart));
                                return { baseName, embeddedIndex + semanticIndex };
                            };

                            for (UINT parameterIndex = 0; parameterIndex < shaderDescription.InputParameters; ++parameterIndex)
                            {
                                D3D11_SIGNATURE_PARAMETER_DESC parameterDescription = {};
                                if (FAILED(shaderReflection->GetInputParameterDesc(parameterIndex, &parameterDescription)))
                                {
                                    continue;
                                }

                                getContext()->log(
                                    Gek::Context::Debug,
                                    "Reflect[{}]: semantic={}{} mask=0x{:X} componentType={} stream={}",
                                    parameterIndex,
                                    parameterDescription.SemanticName,
                                    parameterDescription.SemanticIndex,
                                    uint32_t(parameterDescription.Mask),
                                    uint32_t(parameterDescription.ComponentType),
                                    parameterDescription.Stream);

                                size_t matchedIndex = d3dElementList.size();

                                auto reflectedSemantic = splitSemantic(parameterDescription.SemanticName, parameterDescription.SemanticIndex);
                                for (size_t elementIndex = 0; elementIndex < d3dElementList.size(); ++elementIndex)
                                {
                                    if (usedElement[elementIndex])
                                    {
                                        continue;
                                    }

                                    auto const &candidate = d3dElementList[elementIndex];
                                    auto candidateSemantic = splitSemantic(candidate.SemanticName, candidate.SemanticIndex);
                                    if ((_stricmp(candidateSemantic.first.c_str(), reflectedSemantic.first.c_str()) == 0) && (candidateSemantic.second == reflectedSemantic.second))
                                    {
                                        matchedIndex = elementIndex;
                                        break;
                                    }
                                }

                                if (matchedIndex == d3dElementList.size() && parameterIndex < d3dElementList.size() && !usedElement[parameterIndex])
                                {
                                    matchedIndex = parameterIndex;
                                }

                                if (matchedIndex != d3dElementList.size())
                                {
                                    usedElement[matchedIndex] = true;
                                    D3D11_INPUT_ELEMENT_DESC reflectedElement = d3dElementList[matchedIndex];
                                    reflectedSemanticNames.emplace_back(parameterDescription.SemanticName);
                                    reflectedElement.SemanticName = reflectedSemanticNames.back().c_str();
                                    reflectedElement.SemanticIndex = parameterDescription.SemanticIndex;
                                    reflectedElementList.push_back(reflectedElement);
                                }
                                else
                                {
                                    getContext()->log(Gek::Context::Warning, "Input layout reflection mismatch: missing semantic '{}{}'", parameterDescription.SemanticName, parameterDescription.SemanticIndex);
                                }
                            }

                            if (!reflectedElementList.empty())
                            {
                                CComPtr<ID3D11InputLayout> reflectedInputLayout;
                                HRESULT reflectedCreateResult = d3dDevice->CreateInputLayout(
                                    reflectedElementList.data(),
                                    UINT(reflectedElementList.size()),
                                    information.compiledData.data(),
                                    information.compiledData.size(),
                                    &reflectedInputLayout);

                                if (SUCCEEDED(reflectedCreateResult) && reflectedInputLayout)
                                {
                                    getContext()->log(Gek::Context::Warning, "Recovered input layout creation for program '{}' using shader-reflection signature mapping.", information.name);
                                    return std::make_unique<InputLayout>(reflectedInputLayout);
                                }
                                else
                                {
                                    getContext()->log(Gek::Context::Error, "Reflected input layout creation still failed for program '{}' (HRESULT={})", information.name, getHRESULTString(reflectedCreateResult));
                                }
                            }
                        }
                    }

                    return nullptr;
                }

                return std::make_unique<InputLayout>(d3dInputLayout);
            }

            class Include
                : public ID3DInclude
            {
              private:
                using IncludeFunction = std::function<bool(Render::IncludeType includeType, std::string_view fileName, void const **data, uint32_t *size)>;
                IncludeFunction function;

              public:
                Include(IncludeFunction &&function)
                    : function(std::move(function))
                {
                }

                // ID3DInclude
                ULONG AddRef(void)
                {
                    return 1;
                }

                HRESULT QueryInterface(IID const &interfaceType, void **object)
                {
                    return E_FAIL;
                }

                ULONG Release(void)
                {
                    return 1;
                }

                STDMETHOD(Close)(LPCVOID pData)
                {
                    return S_OK;
                }

                STDMETHOD(Open)(D3D_INCLUDE_TYPE includeType, char const *fileName, void const *parentData, void const **data, uint32_t *size)
                {
                    return function(includeType == D3D_INCLUDE_LOCAL ? Render::IncludeType::Local : Render::IncludeType::Global, fileName, data, size) ? S_OK : E_FAIL;
                }
            };

            bool compileProgram(Render::Program::Information & information, std::function<bool(IncludeType, std::string_view, void const **data, uint32_t *size)> &&onInclude = nullptr)
            {
                assert(d3dDevice);

                if (!information.isValid())
                {
                    return false;
                }

                // Map program type to Slang SM5.0 shader model profiles for DXBC output.
                static const std::unordered_map<Render::Program::Type, std::string_view> D3DTypeMap = {
                    {
                        Render::Program::Type::Compute,
                        "cs_5_0",
                    },
                    {
                        Render::Program::Type::Geometry,
                        "gs_5_0",
                    },
                    {
                        Render::Program::Type::Vertex,
                        "vs_5_0",
                    },
                    {
                        Render::Program::Type::Pixel,
                        "ps_5_0",
                    },
                };

                static const std::vector<uint8_t> EmptyBuffer;

                std::function<std::string(std::string_view, uint32_t)> resolveIncludes;
                resolveIncludes = [&](std::string_view source, uint32_t depth) -> std::string
                {
                    if (depth > 32)
                    {
                        return std::string(source);
                    }

                    std::string resolved;
                    resolved.reserve(source.size());

                    auto tryLoadInclude = [&](Render::IncludeType includeType, std::string const &includeName, void const **includeData, uint32_t *includeSize) -> bool
                    {
                        if (!onInclude || includeName.empty())
                        {
                            return false;
                        }

                        auto addUnique = [](std::vector<std::string> &list, std::string const &candidate)
                        {
                            if (candidate.empty())
                            {
                                return;
                            }

                            if (std::find(list.begin(), list.end(), candidate) == list.end())
                            {
                                list.push_back(candidate);
                            }
                        };

                        std::vector<std::string> candidates;
                        addUnique(candidates, includeName);

                        std::string forwardSlashes = includeName;
                        for (char &character : forwardSlashes)
                        {
                            if (character == '\\')
                            {
                                character = '/';
                            }
                        }
                        addUnique(candidates, forwardSlashes);

                        std::string backSlashes = includeName;
                        for (char &character : backSlashes)
                        {
                            if (character == '/')
                            {
                                character = '\\';
                            }
                        }
                        addUnique(candidates, backSlashes);

                        size_t slashPosition = includeName.find_last_of("/\\");
                        size_t extensionPosition = includeName.find_last_of('.');
                        bool hasExtension = (extensionPosition != std::string::npos) && (slashPosition == std::string::npos || extensionPosition > slashPosition);

                        if (!hasExtension)
                        {
                            addUnique(candidates, includeName + ".slang");
                            addUnique(candidates, forwardSlashes + ".slang");
                            addUnique(candidates, backSlashes + ".slang");
                        }

                        for (auto const &candidate : candidates)
                        {
                            if (onInclude(includeType, candidate, includeData, includeSize) && (*includeData) && (*includeSize) > 0)
                            {
                                return true;
                            }
                        }

                        return false;
                    };

                    size_t cursor = 0;
                    while (cursor < source.size())
                    {
                        size_t lineEnd = source.find('\n', cursor);
                        if (lineEnd == std::string_view::npos)
                        {
                            lineEnd = source.size();
                        }

                        std::string_view line = source.substr(cursor, lineEnd - cursor);

                        size_t scan = 0;
                        while (scan < line.size() && (line[scan] == ' ' || line[scan] == '\t'))
                        {
                            ++scan;
                        }

                        size_t includePos = std::string_view::npos;
                        if (scan < line.size() && line[scan] == '#')
                        {
                            ++scan;
                            while (scan < line.size() && (line[scan] == ' ' || line[scan] == '\t'))
                            {
                                ++scan;
                            }

                            if ((scan + 7) <= line.size() && line.substr(scan, 7) == "include")
                            {
                                includePos = scan;
                            }
                        }

                        if (includePos != std::string_view::npos)
                        {
                            size_t openAngle = line.find('<', includePos);
                            size_t closeAngle = (openAngle != std::string_view::npos) ? line.find('>', openAngle + 1) : std::string_view::npos;
                            size_t openQuote = line.find('"', includePos);
                            size_t closeQuote = (openQuote != std::string_view::npos) ? line.find('"', openQuote + 1) : std::string_view::npos;

                            bool isGlobal = (openAngle != std::string_view::npos && closeAngle != std::string_view::npos);
                            bool isLocal = (openQuote != std::string_view::npos && closeQuote != std::string_view::npos);
                            if (isGlobal || isLocal)
                            {
                                size_t begin = isGlobal ? (openAngle + 1) : (openQuote + 1);
                                size_t end = isGlobal ? closeAngle : closeQuote;
                                std::string includeName(line.substr(begin, end - begin));
                                Render::IncludeType includeType = isGlobal ? Render::IncludeType::Global : Render::IncludeType::Local;

                                const void *includeData = nullptr;
                                uint32_t includeSize = 0;
                                bool includeLoaded = tryLoadInclude(includeType, includeName, &includeData, &includeSize);

                                if (includeLoaded && includeData && includeSize > 0)
                                {
                                    std::string_view includeText(reinterpret_cast<const char *>(includeData), includeSize);
                                    resolved += resolveIncludes(includeText, depth + 1);
                                    if (lineEnd < source.size())
                                    {
                                        resolved.push_back('\n');
                                    }
                                    cursor = (lineEnd < source.size()) ? (lineEnd + 1) : lineEnd;
                                    continue;
                                }
                            }
                        }

                        resolved.append(line.data(), line.size());
                        if (lineEnd < source.size())
                        {
                            resolved.push_back('\n');
                        }
                        cursor = (lineEnd < source.size()) ? (lineEnd + 1) : lineEnd;
                    }

                    return resolved;
                };

                std::string resolvedProgram = resolveIncludes(information.shaderData, 0);
                auto typeSearch = D3DTypeMap.find(information.type);
                if (typeSearch != std::end(D3DTypeMap))
                {
                    Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
                    if (SLANG_SUCCEEDED(slang::createGlobalSession(slangGlobalSession.writeRef())))
                    {
                        slang::TargetDesc targetDesc = {};
                        // Directly generate DXBC from Slang for D3D11.
                        targetDesc.format = SLANG_DXBC;

                        // Use the mapped profile for the shader stage (e.g. "vs_5_0").
                        SlangProfileID profile = SLANG_PROFILE_UNKNOWN;
                        const std::string mappedProfile = std::string(typeSearch->second);
                        profile = slangGlobalSession->findProfile(mappedProfile.c_str());
                        if (!profile || profile == SLANG_PROFILE_UNKNOWN)
                        {
                            getContext()->log(Gek::Context::Warning, "Requested Slang profile '{}' not available while compiling '{}'; continuing without explicit Slang profile.", mappedProfile, information.name);
                            profile = SLANG_PROFILE_UNKNOWN;
                        }

                        targetDesc.profile = profile;

                        getContext()->log(Gek::Context::Debug, "Using Slang DXBC profile ('{}') while compiling '{}'.", mappedProfile, information.name);

                        slang::SessionDesc sessionDesc = {};
                        sessionDesc.targets = &targetDesc;
                        sessionDesc.targetCount = 1;

                        Slang::ComPtr<slang::ISession> session;
                        if (SLANG_SUCCEEDED(slangGlobalSession->createSession(sessionDesc, session.writeRef())) && session)
                        {
                            getContext()->log(Gek::Context::Debug, "Loading Slang module for '{}' with entry point '{}'.", information.name, information.entryFunction);

                            slang::IBlob *outDiagnosticsRaw = nullptr;
                            slang::IModule *slangModule = session->loadModuleFromSourceString(information.name.data(), information.shaderPath.getFileName().data(), resolvedProgram.c_str(), &outDiagnosticsRaw);
                            Slang::ComPtr<slang::IBlob> outDiagnostics(outDiagnosticsRaw);

                            if (outDiagnostics)
                            {
                                const char *diagnosticMessage = reinterpret_cast<const char *>(outDiagnostics->getBufferPointer());
                                getContext()->log(Gek::Context::Debug, "Slang module load diagnostics: {}", diagnosticMessage);
                            }

                            if (slangModule)
                            {
                                getContext()->log(Gek::Context::Debug, "Module loaded successfully, searching for entry point '{}'.", information.entryFunction);

                                Slang::ComPtr<slang::IEntryPoint> entryPoint;
                                slangModule->findEntryPointByName(information.entryFunction.data(), entryPoint.writeRef());
                                if (entryPoint)
                                {
                                    getContext()->log(Gek::Context::Debug, "Entry point found; composing program.");

                                    std::vector<slang::IComponentType *> componentTypes;
                                    componentTypes.push_back(slangModule);
                                    componentTypes.push_back(entryPoint);

                                    Slang::ComPtr<slang::IComponentType> composedProgram;
                                    slang::IBlob *compositeDiagnosticsRaw = nullptr;
                                    session->createCompositeComponentType(componentTypes.data(), componentTypes.size(), composedProgram.writeRef(), &compositeDiagnosticsRaw);
                                    Slang::ComPtr<slang::IBlob> compositeDiagnostics(compositeDiagnosticsRaw);
                                    if (composedProgram)
                                    {
                                        Slang::ComPtr<slang::IBlob> dxbcCode;
                                        SlangResult codeResult = composedProgram->getEntryPointCode(0, 0, dxbcCode.writeRef());
                                        if (SLANG_SUCCEEDED(codeResult) && dxbcCode)
                                        {
                                            const uint8_t *data = reinterpret_cast<const uint8_t *>(dxbcCode->getBufferPointer());
                                            const size_t dataSize = dxbcCode->getBufferSize();
                                            if (data && dataSize > 0)
                                            {
                                                information.compiledData.assign(data, data + dataSize);
                                                getContext()->log(Gek::Context::Info, "Slang->DXBC succeeded for '{}' entry '{}' ({} bytes)", information.name, information.entryFunction, information.compiledData.size());
                                                return true;
                                            }
                                        }
                                        else
                                        {
                                            getContext()->log(Gek::Context::Error, "Slang DXBC generation failed for '{}': getEntryPointCode returned error code {}", information.name, static_cast<int32_t>(codeResult));
                                        }
                                    }
                                    else
                                    {
                                        const char *diagnosticMessage = compositeDiagnostics ? reinterpret_cast<const char *>(compositeDiagnostics->getBufferPointer()) : "Unknown Slang composite error";
                                        getContext()->log(Gek::Context::Error, "Slang composite program failed for '{}': {}", information.name, diagnosticMessage);
                                    }
                                }
                                else
                                {
                                    const char *diagnosticMessage = outDiagnostics ? reinterpret_cast<const char *>(outDiagnostics->getBufferPointer()) : "Unknown Slang entry point error";
                                    getContext()->log(Gek::Context::Error, "Slang entry point '{}' not found in module '{}': {}", information.entryFunction, information.name, diagnosticMessage);
                                }
                            }
                            else
                            {
                                const char *diagnosticMessage = outDiagnostics ? reinterpret_cast<const char *>(outDiagnostics->getBufferPointer()) : "Unknown Slang loadModule error";
                                getContext()->log(Gek::Context::Error, "Failed to load Slang module for '{}': {}", information.name, diagnosticMessage);
                            }
                        }
                    }
                }

                return false;
            }

            template <class D3DTYPE, class TYPE, typename RETURN, typename CLASS, typename... PARAMETERS>
            Render::ProgramPtr createProgram(Render::Program::Information const &information, RETURN (__stdcall CLASS::*function)(PARAMETERS...))
            {
                assert(function);

                if (information.compiledData.empty())
                {
                    return nullptr;
                }

                CComPtr<D3DTYPE> d3dShader;
                HRESULT resultValue = (d3dDevice->*function)(information.compiledData.data(), information.compiledData.size(), nullptr, &d3dShader);
                if (FAILED(resultValue) || !d3dShader)
                {
                    return nullptr;
                }

                getContext()->log(Gek::Context::Debug, "D3D shader created successfully: {} bytes", information.compiledData.size());

                return std::make_unique<TYPE>(d3dShader, information);
            }

            Render::ProgramPtr createProgram(Render::Program::Information const &information)
            {
                switch (information.type)
                {
                case Render::Program::Type::Compute:
                    return createProgram<ID3D11ComputeShader, ComputeProgram>(information, &ID3D11Device::CreateComputeShader);

                case Render::Program::Type::Vertex:
                    return createProgram<ID3D11VertexShader, VertexProgram>(information, &ID3D11Device::CreateVertexShader);

                case Render::Program::Type::Geometry:
                    return createProgram<ID3D11GeometryShader, GeometryProgram>(information, &ID3D11Device::CreateGeometryShader);

                case Render::Program::Type::Pixel:
                    return createProgram<ID3D11PixelShader, PixelProgram>(information, &ID3D11Device::CreatePixelShader);
                };

                getContext()->log(Gek::Context::Error, "Unknown program pipeline encountered");
                return nullptr;
            }

            Render::TexturePtr createTexture(const Render::Texture::Description &description, const void *data)
            {
                assert(d3dDevice);

                if (description.format == Render::Format::Unknown ||
                    description.width == 0 ||
                    description.height == 0 ||
                    description.depth == 0)
                {
                    return nullptr;
                }

                Render::Texture::Description finalDescription = description;

                uint32_t bindFlags = 0;
                if (description.flags & Render::Texture::Flags::RenderTarget)
                {
                    if (description.flags & Render::Texture::Flags::DepthTarget)
                    {
                        getContext()->log(Gek::Context::Error, "Cannot create render target when depth target flag also specified");
                        return nullptr;
                    }

                    bindFlags |= D3D11_BIND_RENDER_TARGET;
                }

                if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    if (description.depth > 1)
                    {
                        getContext()->log(Gek::Context::Error, "Depth target must have depth of one");
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
                resourceData.SysMemPitch = (Render::Implementation::FormatStrideList[static_cast<uint8_t>(description.format)] * description.width);
                resourceData.SysMemSlicePitch = (description.depth == 1 ? 0 : (resourceData.SysMemPitch * description.height));

                CComQIPtr<ID3D11Resource> d3dResource;
                if (description.depth == 1)
                {
                    D3D11_TEXTURE2D_DESC textureDescription;
                    textureDescription.Width = description.width;
                    textureDescription.Height = description.height;
                    textureDescription.MipLevels = description.mipMapCount;
                    textureDescription.Format = Render::Implementation::TextureFormatList[static_cast<uint8_t>(description.format)];
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
                        getContext()->log(Gek::Context::Error, "Unable to create 2D texture");
                        return nullptr;
                    }

                    texture2D->GetDesc(&textureDescription);
                    finalDescription.mipMapCount = textureDescription.MipLevels;
                    d3dResource = texture2D;
                }
                else
                {
                    D3D11_TEXTURE3D_DESC textureDescription;
                    textureDescription.Width = description.width;
                    textureDescription.Height = description.height;
                    textureDescription.Depth = description.depth;
                    textureDescription.MipLevels = description.mipMapCount;
                    textureDescription.Format = Render::Implementation::TextureFormatList[static_cast<uint8_t>(description.format)];
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
                        getContext()->log(Gek::Context::Error, "Unable to create 3D texture");
                        return nullptr;
                    }

                    texture3D->GetDesc(&textureDescription);
                    finalDescription.mipMapCount = textureDescription.MipLevels;
                    d3dResource = texture3D;
                }

                if (!d3dResource)
                {
                    getContext()->log(Gek::Context::Error, "Unable to get texture resource");
                    return nullptr;
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Render::Texture::Flags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = Render::Implementation::ViewFormatList[static_cast<uint8_t>(description.format)];
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
                        getContext()->log(Gek::Context::Error, "Unable to create texture shader resource view");
                        return nullptr;
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Render::Texture::Flags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = Render::Implementation::ViewFormatList[static_cast<uint8_t>(description.format)];
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
                        getContext()->log(Gek::Context::Error, "Unable to create texture unordered access view");
                        return nullptr;
                    }
                }

                if (description.flags & Render::Texture::Flags::RenderTarget)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                    renderViewDescription.Format = Render::Implementation::ViewFormatList[static_cast<uint8_t>(description.format)];
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
                        getContext()->log(Gek::Context::Error, "Unable to create render target view");
                        return nullptr;
                    }

                    return std::make_unique<UnorderedTargetViewTexture>(d3dResource, d3dRenderTargetView, d3dShaderResourceView, d3dUnorderedAccessView, description);
                }
                else if (description.flags & Render::Texture::Flags::DepthTarget)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = Render::Implementation::DepthFormatList[static_cast<uint8_t>(description.format)];
                    depthStencilDescription.ViewDimension = (description.sampleCount > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D);
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
                    HRESULT resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &d3dDepthStencilView);
                    if (FAILED(resultValue) || !d3dDepthStencilView)
                    {
                        getContext()->log(Gek::Context::Error, "Unable to create depth stencil view");
                        return nullptr;
                    }

                    return std::make_unique<DepthTexture>(d3dResource, d3dDepthStencilView, d3dShaderResourceView, d3dUnorderedAccessView, description);
                }
                else
                {
                    return std::make_unique<UnorderedViewTexture>(d3dResource, d3dShaderResourceView, d3dUnorderedAccessView, description);
                }
            }

            Render::TexturePtr loadTextureFromKtx2(std::vector<uint8_t> const &fileData, FileSystem::Path const &filePath)
            {
                ktxTexture2 *kTexture = nullptr;
                KTX_error_code ktxResult = ktxTexture2_CreateFromMemory(fileData.data(), fileData.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kTexture);
                if (ktxResult != KTX_SUCCESS || !kTexture)
                {
                    getContext()->log(Gek::Context::Error, "D3D11 loadTexture failed: KTX2 decode error for '{}'", filePath.getString());
                    return nullptr;
                }

                DXGI_FORMAT dxgiFormat = VkFormatToDxgi(kTexture->vkFormat);
                if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
                {
                    getContext()->log(Gek::Context::Error, "D3D11 loadTexture failed: unsupported KTX2 vkFormat {} for '{}'", kTexture->vkFormat, filePath.getString());
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    return nullptr;
                }

                uint32_t mipLevels = kTexture->numLevels;
                uint32_t width = kTexture->baseWidth;
                uint32_t height = kTexture->baseHeight;

                if (!validateTextureForD3D11(getContext(), d3dDevice, dxgiFormat, width, height))
                {
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    return nullptr;
                }

                std::vector<D3D11_SUBRESOURCE_DATA> subresources;
                subresources.reserve(mipLevels);
                for (uint32_t mip = 0; mip < mipLevels; ++mip)
                {
                    ktx_size_t offset = 0;
                    ktxTexture_GetImageOffset(ktxTexture(kTexture), mip, 0, 0, &offset);
                    ktx_size_t imageSize = ktxTexture_GetImageSize(ktxTexture(kTexture), mip);
                    uint32_t mipWidth = std::max(width >> mip, 1u);
                    // For BCn formats, pitch is in 4x4 blocks
                    uint32_t blockWidth = (mipWidth + 3) / 4;
                    uint32_t blockBytes = (dxgiFormat == DXGI_FORMAT_BC1_UNORM || dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB || dxgiFormat == DXGI_FORMAT_BC4_UNORM || dxgiFormat == DXGI_FORMAT_BC4_SNORM) ? 8 : 16;
                    D3D11_SUBRESOURCE_DATA srd = {};
                    srd.pSysMem = kTexture->pData + offset;
                    srd.SysMemPitch = blockWidth * blockBytes;
                    srd.SysMemSlicePitch = static_cast<UINT>(imageSize);
                    subresources.push_back(srd);
                }

                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = width;
                desc.Height = height;
                desc.MipLevels = mipLevels;
                desc.ArraySize = 1;
                desc.Format = dxgiFormat;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                CComPtr<ID3D11Texture2D> d3dTexture;
                HRESULT hr = d3dDevice->CreateTexture2D(&desc, subresources.data(), &d3dTexture);
                ktxTexture_Destroy(ktxTexture(kTexture));
                if (FAILED(hr) || !d3dTexture)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateTexture2D from KTX2", hr);
                    return nullptr;
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                hr = d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, &d3dShaderResourceView);
                if (FAILED(hr) || !d3dShaderResourceView)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateShaderResourceView from KTX2", hr);
                    return nullptr;
                }

                Render::Texture::Description description;
                description.width = width;
                description.height = height;
                description.depth = 1;
                description.format = Render::Implementation::GetFormat(dxgiFormat);
                description.mipMapCount = mipLevels;
                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                return std::make_unique<ViewTexture>(d3dResource, d3dShaderResourceView, description);
            }

            Render::TexturePtr loadTexture(FileSystem::Path const &filePath, uint32_t flags)
            {
                assert(d3dDevice);

                std::vector<uint8_t> fileData(FileSystem::Load(filePath));
                if (fileData.empty())
                {
                    getContext()->log(Gek::Context::Error, "Unable to load data from texture file: {}", filePath.getString());
                    return nullptr;
                }

                std::string extension(String::GetLower(filePath.getExtension()));
                if (extension == ".ktx2")
                {
                    return loadTextureFromKtx2(fileData, filePath);
                }

                int width = 0, height = 0, channels = 0;
                stbi_uc *pixels = stbi_load_from_memory(fileData.data(), static_cast<int>(fileData.size()), &width, &height, &channels, 4);
                if (!pixels)
                {
                    getContext()->log(Gek::Context::Error, "stb_image failed to decode texture: {}", filePath.getString());
                    return nullptr;
                }

                DXGI_FORMAT dxgiFormat = (flags & Render::TextureLoadFlags::sRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                if (!validateTextureForD3D11(getContext(), d3dDevice, dxgiFormat, static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
                {
                    stbi_image_free(pixels);
                    return nullptr;
                }

                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = static_cast<UINT>(width);
                desc.Height = static_cast<UINT>(height);
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = dxgiFormat;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA initData = {};
                initData.pSysMem = pixels;
                initData.SysMemPitch = static_cast<UINT>(width) * 4;

                CComPtr<ID3D11Texture2D> d3dTexture;
                HRESULT hr = d3dDevice->CreateTexture2D(&desc, &initData, &d3dTexture);
                stbi_image_free(pixels);
                if (FAILED(hr) || !d3dTexture)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateTexture2D from stb_image", hr);
                    return nullptr;
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                hr = d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, &d3dShaderResourceView);
                if (FAILED(hr) || !d3dShaderResourceView)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateShaderResourceView from stb_image", hr);
                    return nullptr;
                }

                Render::Texture::Description description;
                description.width = static_cast<uint32_t>(width);
                description.height = static_cast<uint32_t>(height);
                description.depth = 1;
                description.format = Render::Implementation::GetFormat(dxgiFormat);
                description.mipMapCount = 1;
                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                return std::make_unique<ViewTexture>(d3dResource, d3dShaderResourceView, description);
            }

            Render::TexturePtr loadTexture(void const *buffer, size_t size, uint32_t flags)
            {
                int width = 0, height = 0, channels = 0;
                stbi_uc *pixels = stbi_load_from_memory(static_cast<stbi_uc const *>(buffer), static_cast<int>(size), &width, &height, &channels, 4);
                if (!pixels)
                {
                    getContext()->log(Gek::Context::Error, "stb_image failed to decode in-memory texture");
                    return nullptr;
                }

                DXGI_FORMAT dxgiFormat = (flags & Render::TextureLoadFlags::sRGB) ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
                if (!validateTextureForD3D11(getContext(), d3dDevice, dxgiFormat, static_cast<uint32_t>(width), static_cast<uint32_t>(height)))
                {
                    stbi_image_free(pixels);
                    return nullptr;
                }

                D3D11_TEXTURE2D_DESC desc = {};
                desc.Width = static_cast<UINT>(width);
                desc.Height = static_cast<UINT>(height);
                desc.MipLevels = 1;
                desc.ArraySize = 1;
                desc.Format = dxgiFormat;
                desc.SampleDesc.Count = 1;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

                D3D11_SUBRESOURCE_DATA initData = {};
                initData.pSysMem = pixels;
                initData.SysMemPitch = static_cast<UINT>(width) * 4;

                CComPtr<ID3D11Texture2D> d3dTexture;
                HRESULT hr = d3dDevice->CreateTexture2D(&desc, &initData, &d3dTexture);
                stbi_image_free(pixels);
                if (FAILED(hr) || !d3dTexture)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateTexture2D from stb_image (memory)", hr);
                    return nullptr;
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                hr = d3dDevice->CreateShaderResourceView(d3dTexture, nullptr, &d3dShaderResourceView);
                if (FAILED(hr) || !d3dShaderResourceView)
                {
                    logD3D11Failure(getContext(), d3dDevice, "CreateShaderResourceView from stb_image (memory)", hr);
                    return nullptr;
                }

                Render::Texture::Description description;
                description.width = static_cast<uint32_t>(width);
                description.height = static_cast<uint32_t>(height);
                description.depth = 1;
                description.format = Render::Implementation::GetFormat(dxgiFormat);
                description.mipMapCount = 1;
                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                return std::make_unique<ViewTexture>(d3dResource, d3dShaderResourceView, description);
            }

            Texture::Description loadTextureDescription(FileSystem::Path const &filePath)
            {
                static const Texture::Description EmptyDescription;

                std::string extension(String::GetLower(filePath.getExtension()));
                if (extension == ".ktx2")
                {
                    std::vector<uint8_t> buffer(FileSystem::Load(filePath));
                    if (buffer.empty())
                    {
                        getContext()->log(Gek::Context::Error, "Unable to load KTX2 file for description: {}", filePath.getString());
                        return EmptyDescription;
                    }

                    ktxTexture2 *kTexture = nullptr;
                    KTX_error_code ktxResult = ktxTexture2_CreateFromMemory(buffer.data(), buffer.size(), KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT, &kTexture);
                    if (ktxResult != KTX_SUCCESS || !kTexture)
                    {
                        getContext()->log(Gek::Context::Error, "KTX2 header parse failed for: {}", filePath.getString());
                        return EmptyDescription;
                    }

                    Texture::Description description;
                    description.width = kTexture->baseWidth;
                    description.height = kTexture->baseHeight;
                    description.depth = kTexture->baseDepth;
                    description.mipMapCount = kTexture->numLevels;
                    description.format = Render::Implementation::GetFormat(VkFormatToDxgi(kTexture->vkFormat));
                    ktxTexture_Destroy(ktxTexture(kTexture));
                    return description;
                }

                // For raster images, use stb_image header-only probe
                std::vector<uint8_t> buffer(FileSystem::Load(filePath, 1024 * 4));
                if (buffer.empty())
                {
                    getContext()->log(Gek::Context::Error, "Unable to load data from texture file: {}", filePath.getString());
                    return EmptyDescription;
                }

                int width = 0, height = 0, channels = 0;
                if (!stbi_info_from_memory(buffer.data(), static_cast<int>(buffer.size()), &width, &height, &channels))
                {
                    getContext()->log(Gek::Context::Error, "stb_image could not read texture header: {}", filePath.getString());
                    return EmptyDescription;
                }

                Texture::Description description;
                description.width = static_cast<uint32_t>(width);
                description.height = static_cast<uint32_t>(height);
                description.depth = 1;
                description.mipMapCount = 1;
                description.format = Render::Format::R8G8B8A8_UNORM;
                return description;
            }

            void executeCommandList(Render::Object * commandList)
            {
                assert(d3dDeviceContext);
                assert(commandList);

                CComQIPtr<ID3D11CommandList> d3dCommandList;
                d3dDeviceContext->ExecuteCommandList(getObject<CommandList>(commandList), FALSE);
            }

            void present(bool waitForVerticalSync)
            {
                assert(dxgiSwapChain);

                const auto presentStartTime = std::chrono::high_resolution_clock::now();
                dxgiSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
                const auto presentEndTime = std::chrono::high_resolution_clock::now();
                const double presentCpuMs = std::chrono::duration<double, std::milli>(presentEndTime - presentStartTime).count();

                ++sampleFrameIndex;
                getContext()->setRuntimeMetric("d3d11.frame", static_cast<double>(sampleFrameIndex));
                getContext()->setRuntimeMetric("render.frame", static_cast<double>(sampleFrameIndex));
                getContext()->setRuntimeMetric("render.backend", 1.0);
                getContext()->setRuntimeMetric("d3d11.presentCpuMs", presentCpuMs);
                getContext()->setRuntimeMetric("render.presentCpuMs", presentCpuMs);
                loggedFirstIndexedStateThisFrame = false;
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // namespace Render::Implementation
}; // namespace Gek
