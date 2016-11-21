#pragma warning(disable : 4005)

#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include "GEK\Utility\FileSystem.hpp"
#include "GEK\Utility\ContextUser.hpp"
#include "GEK\System\VideoDevice.hpp"
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

namespace Gek
{
    // Function Traits
    // http://stackoverflow.com/questions/2562320/specializing-a-template-on-a-lambda-in-c0x

    template <typename CLASS>
    struct FunctionCache
    {
        using ReturnType = typename FunctionCache<decltype(&CLASS::operator())>::ReturnType;
        using ArgumentTypes = typename FunctionCache<decltype(&CLASS::operator())>::ArgumentTypes;
    };

    template <typename RETURN, typename CLASS, typename... PARAMETERS>
    struct FunctionCache<RETURN(__stdcall CLASS::*)(PARAMETERS...)>
    {
        using ReturnType = RETURN;
        using ArgumentTypes = std::tuple<typename std::decay<PARAMETERS>::type...>;

        ArgumentTypes cache;
        void operator()(CLASS *classObject, RETURN(__stdcall CLASS::*function)(PARAMETERS...), PARAMETERS... arguments)
        {
            auto current = std::tie(arguments...);
            if (current != cache)
            {
                cache = current;
                (classObject->*function)(arguments...);
            }
        }
    };

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

        static_assert(ARRAYSIZE(TextureFormatList) == static_cast<uint8_t>(Video::Format::Count), "New format added without adding to all TextureFormatList.");

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

        static_assert(ARRAYSIZE(DepthFormatList) == static_cast<uint8_t>(Video::Format::Count), "New format added without adding to all DepthFormatList.");

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

        static_assert(ARRAYSIZE(ViewFormatList) == static_cast<uint8_t>(Video::Format::Count), "New format added without adding to all ViewFormatList.");

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

        static_assert(ARRAYSIZE(BufferFormatList) == static_cast<uint8_t>(Video::Format::Count), "New format added without adding to all BufferFormatList.");

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

        static_assert(ARRAYSIZE(FormatStrideList) == static_cast<uint8_t>(Video::Format::Count), "New format added without adding to all FormatStrideList.");

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

        static const char *SemanticNameList[] =
        {
            "POSITION",
            "TEXCOORD",
            "TANGENT",
            "BINORMAL",
            "NORMAL",
            "COLOR",
        };

        static_assert(ARRAYSIZE(SemanticNameList) == static_cast<uint8_t>(Video::InputElement::Semantic::Count), "New input element semantic added without adding to all SemanticNameList.");

        Video::Format getFormat(DXGI_FORMAT format)
        {
            switch (format)
            {
            case DXGI_FORMAT_R32G32B32A32_FLOAT: return Video::Format::R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R16G16B16A16_FLOAT: return Video::Format::R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32B32_FLOAT: return Video::Format::R32G32B32_FLOAT;
            case DXGI_FORMAT_R11G11B10_FLOAT: return Video::Format::R11G11B10_FLOAT;
            case DXGI_FORMAT_R32G32_FLOAT: return Video::Format::R32G32_FLOAT;
            case DXGI_FORMAT_R16G16_FLOAT: return Video::Format::R16G16_FLOAT;
            case DXGI_FORMAT_R32_FLOAT: return Video::Format::R32_FLOAT;
            case DXGI_FORMAT_R16_FLOAT: return Video::Format::R16_FLOAT;

            case DXGI_FORMAT_R32G32B32A32_UINT: return Video::Format::R32G32B32A32_UINT;
            case DXGI_FORMAT_R16G16B16A16_UINT: return Video::Format::R16G16B16A16_UINT;
            case DXGI_FORMAT_R10G10B10A2_UINT: return Video::Format::R10G10B10A2_UINT;
            case DXGI_FORMAT_R8G8B8A8_UINT: return Video::Format::R8G8B8A8_UINT;
            case DXGI_FORMAT_R32G32B32_UINT: return Video::Format::R32G32B32_UINT;
            case DXGI_FORMAT_R32G32_UINT: return Video::Format::R32G32_UINT;
            case DXGI_FORMAT_R16G16_UINT: return Video::Format::R16G16_UINT;
            case DXGI_FORMAT_R8G8_UINT: return Video::Format::R8G8_UINT;
            case DXGI_FORMAT_R32_UINT: return Video::Format::R32_UINT;
            case DXGI_FORMAT_R16_UINT: return Video::Format::R16_UINT;
            case DXGI_FORMAT_R8_UINT: return Video::Format::R8_UINT;

            case DXGI_FORMAT_R32G32B32A32_SINT: return Video::Format::R32G32B32A32_INT;
            case DXGI_FORMAT_R16G16B16A16_SINT: return Video::Format::R16G16B16A16_INT;
            case DXGI_FORMAT_R8G8B8A8_SINT: return Video::Format::R8G8B8A8_INT;
            case DXGI_FORMAT_R32G32B32_SINT: return Video::Format::R32G32B32_INT;
            case DXGI_FORMAT_R32G32_SINT: return Video::Format::R32G32_INT;
            case DXGI_FORMAT_R16G16_SINT: return Video::Format::R16G16_INT;
            case DXGI_FORMAT_R8G8_SINT: return Video::Format::R8G8_INT;
            case DXGI_FORMAT_R32_SINT: return Video::Format::R32_INT;
            case DXGI_FORMAT_R16_SINT: return Video::Format::R16_INT;
            case DXGI_FORMAT_R8_SINT: return Video::Format::R8_INT;

            case DXGI_FORMAT_R16G16B16A16_UNORM: return Video::Format::R16G16B16A16_UNORM;
            case DXGI_FORMAT_R10G10B10A2_UNORM: return Video::Format::R10G10B10A2_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM: return Video::Format::R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Video::Format::R8G8B8A8_UNORM_SRGB;
            case DXGI_FORMAT_R16G16_UNORM: return Video::Format::R16G16_UNORM;
            case DXGI_FORMAT_R8G8_UNORM: return Video::Format::R8G8_UNORM;
            case DXGI_FORMAT_R16_UNORM: return Video::Format::R16_UNORM;
            case DXGI_FORMAT_R8_UNORM: return Video::Format::R8_UNORM;

            case DXGI_FORMAT_R16G16B16A16_SNORM: return Video::Format::R16G16B16A16_NORM;
            case DXGI_FORMAT_R8G8B8A8_SNORM: return Video::Format::R8G8B8A8_NORM;
            case DXGI_FORMAT_R16G16_SNORM: return Video::Format::R16G16_NORM;
            case DXGI_FORMAT_R8G8_SNORM: return Video::Format::R8G8_NORM;
            case DXGI_FORMAT_R16_SNORM: return Video::Format::R16_NORM;
            case DXGI_FORMAT_R8_SNORM: return Video::Format::R8_NORM;
            };

            return Video::Format::Unknown;
        }
    }; // namespace DirectX

    namespace Direct3D11
    {
        template <typename CLASS>
        void setDebugName(CComPtr<CLASS> &object, const wchar_t *name)
        {
            if (object)
            {
                StringUTF8 nameUTF8(name);
                object->SetPrivateData(WKPDID_D3DDebugObjectName, UINT(nameUTF8.length()), nameUTF8.c_str());
            }
        }

        template <typename CONVERT, typename SOURCE>
        auto getObject(SOURCE *source)
        {
            return (source ? dynamic_cast<CONVERT *>(source)->CONVERT::d3dObject.p : nullptr);
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

            TYPE * const *get(void) const
            {
                return objectList.data();
            }
        };

        template <typename TYPE>
        class BaseObject
        {
        public:
            CComPtr<TYPE> d3dObject;

        public:
            template <typename TYPE>
            BaseObject(TYPE *d3dSource)
                : d3dObject(CComQIPtr<TYPE>(d3dSource))
            {
            }

            virtual ~BaseObject(void) = default;
        };

        template <typename TYPE>
        class BaseVideoObject
            : public Video::Object
        {
        public:
            CComPtr<TYPE> d3dObject;

        public:
            template <typename SOURCE>
            BaseVideoObject(const SOURCE &d3dSource)
                : d3dObject(CComQIPtr<TYPE>(d3dSource))
            {
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dObject, name);
            }
        };

        using CommandList = BaseVideoObject<ID3D11CommandList>;
        using RenderState = BaseVideoObject<ID3D11RasterizerState>;
        using DepthState = BaseVideoObject<ID3D11DepthStencilState>;
        using BlendState = BaseVideoObject<ID3D11BlendState>;
        using SamplerState = BaseVideoObject<ID3D11SamplerState>;
        using Event = BaseVideoObject<ID3D11Query>;
        using InputLayout = BaseVideoObject<ID3D11InputLayout>;
        using ComputeProgram = BaseVideoObject<ID3D11ComputeShader>;
        using VertexProgram = BaseVideoObject<ID3D11VertexShader>;
        using GeometryProgram = BaseVideoObject<ID3D11GeometryShader>;
        using PixelProgram = BaseVideoObject<ID3D11PixelShader>;

        using Resource = BaseObject<ID3D11Resource>;
        using ShaderResourceView = BaseObject<ID3D11ShaderResourceView>;
        using UnorderedAccessView = BaseObject<ID3D11UnorderedAccessView>;
        using RenderTargetView = BaseObject<ID3D11RenderTargetView>;

        class Buffer
            : public Video::Buffer
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            CComPtr<ID3D11Buffer> d3dObject;
            Video::BufferDescription description;

        public:
            Buffer(ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, const Video::BufferDescription &description)
                : Resource(d3dBuffer)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
                , description(description)
                , d3dObject(d3dBuffer)
            {
            }

            virtual ~Buffer(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(d3dObject, name);
                setDebugName(Resource::d3dObject, name);
                setDebugName(ShaderResourceView::d3dObject, name);
                setDebugName(UnorderedAccessView::d3dObject, name);
            }

            // Video::Buffer
            const Video::BufferDescription &getDescription(void) const
            {
                return description;
            }
        };

        class BaseTexture
        {
        public:
            Video::TextureDescription description;

        public:
            BaseTexture(const Video::TextureDescription &description)
                : description(description)
            {
            }
        };

        class Texture
            : virtual public Video::Texture
            , public BaseTexture
        {
        public:
            Texture(const Video::TextureDescription &description)
                : BaseTexture(description)
            {
            }

            virtual ~Texture(void) = default;

            // Video::Texture
            const Video::TextureDescription &getDescription(void) const
            {
                return description;
            }
        };

        class ViewTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            ViewTexture(ID3D11Resource *d3dResource, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, const Video::TextureDescription &description)
                : Texture(description)
                , Resource(d3dResource)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            void setName(const wchar_t *name)
            {
                setDebugName(Resource::d3dObject, name);
                setDebugName(ShaderResourceView::d3dObject, name);
                setDebugName(UnorderedAccessView::d3dObject, name);
            }
        };

        class Target
            : virtual public Video::Target
            , public BaseTexture
        {
        public:
            Video::ViewPort viewPort;

        public:
            Target(const Video::TextureDescription &description)
                : BaseTexture(description)
				, viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(description.width), float(description.height)), 0.0f, 1.0f)
			{
            }

            virtual ~Target(void) = default;

            // Video::Texture
            const Video::TextureDescription &getDescription(void) const
            {
                return description;
            }

            // Video::Target
            const Video::ViewPort &getViewPort(void) const
            {
                return viewPort;
            }
        };

        class TargetTexture
            : public Target
            , public Resource
            , public RenderTargetView
        {
        public:
            TargetTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, const Video::TextureDescription &description)
                : Target(description)
                , Resource(d3dResource)
                , RenderTargetView(d3dRenderTargetView)
            {
            }

            virtual ~TargetTexture(void) = default;

            virtual void setName(const wchar_t *name)
            {
                setDebugName(Resource::d3dObject, name);
                setDebugName(RenderTargetView::d3dObject, name);
            }
        };

        class TargetViewTexture
            : virtual public TargetTexture
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            TargetViewTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, const Video::TextureDescription &description)
                : TargetTexture(d3dResource, d3dRenderTargetView, description)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            virtual ~TargetViewTexture(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(Resource::d3dObject, name);
                setDebugName(RenderTargetView::d3dObject, name);
                setDebugName(ShaderResourceView::d3dObject, name);
                setDebugName(UnorderedAccessView::d3dObject, name);
            }
        };

        class DepthTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            CComPtr<ID3D11DepthStencilView> d3dObject;

        public:
            DepthTexture(ID3D11Resource *d3dResource, ID3D11DepthStencilView *d3dDepthStencilView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, const Video::TextureDescription &description)
                : Texture(description)
                , Resource(d3dResource)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
                , d3dObject(d3dDepthStencilView)
            {
            }

            virtual ~DepthTexture(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(Resource::d3dObject, name);
                setDebugName(ShaderResourceView::d3dObject, name);
                setDebugName(UnorderedAccessView::d3dObject, name);
                setDebugName(d3dObject, name);
            }
        };

        GEK_CONTEXT_USER(Device, HWND, Video::DeviceDescription)
            , public Video::Debug::Device
        {
            class Context
                : public Video::Device::Context
            {
                class ComputePipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                public:
                    ComputePipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
                    Video::PipelineType getType(void) const
                    {
                        return Video::PipelineType::Compute;
                    }

                    void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->CSSetShader(getObject<ComputeProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->CSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->CSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->CSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    ObjectCache<ID3D11UnorderedAccessView> unorderedAccessCache;
                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.set<UnorderedAccessView>(list);
                        d3dDeviceContext->CSSetUnorderedAccessViews(firstStage, UINT(list.size()), unorderedAccessCache.get(), countList);
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->CSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->CSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->CSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.clear(count);
                        d3dDeviceContext->CSSetUnorderedAccessViews(firstStage, count, unorderedAccessCache.get(), nullptr);
                    }
                };

                class VertexPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                public:
                    VertexPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
                    Video::PipelineType getType(void) const
                    {
                        return Video::PipelineType::Vertex;
                    }

                    void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->VSSetShader(getObject<VertexProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->VSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->VSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->VSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation("Vertex pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->VSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->VSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->VSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        throw Video::UnsupportedOperation("Vertex pipeline does not supported unordered access");
                    }
                };

                class GeometryPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                public:
                    GeometryPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
                    Video::PipelineType getType(void) const
                    {
                        return Video::PipelineType::Geometry;
                    }

                    void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->GSSetShader(getObject<GeometryProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->GSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->GSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->GSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation("Geometry pipeline does not supported unordered access");
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->GSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->GSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->GSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        throw Video::UnsupportedOperation("Geometry pipeline does not supported unordered access");
                    }
                };

                class PixelPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext = nullptr;

                public:
                    PixelPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
                    Video::PipelineType getType(void) const
                    {
                        return Video::PipelineType::Pixel;
                    }

                    void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->PSSetShader(getObject<PixelProgram>(program), nullptr, 0);
                    }

                    ObjectCache<ID3D11SamplerState> samplerStateCache;
                    void setSamplerStateList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.set<SamplerState>(list);
                        d3dDeviceContext->PSSetSamplers(firstStage, UINT(list.size()), samplerStateCache.get());
                    }

                    ObjectCache<ID3D11Buffer> constantBufferCache;
                    void setConstantBufferList(const std::vector<Video::Buffer *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.set<Buffer>(list);
                        d3dDeviceContext->PSSetConstantBuffers(firstStage, UINT(list.size()), constantBufferCache.get());
                    }

                    ObjectCache<ID3D11ShaderResourceView> resourceCache;
                    void setResourceList(const std::vector<Video::Object *> &list, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.set<ShaderResourceView>(list);
                        d3dDeviceContext->PSSetShaderResources(firstStage, UINT(list.size()), resourceCache.get());
                    }

                    ObjectCache<ID3D11UnorderedAccessView> unorderedAccessCache;
                    void setUnorderedAccessList(const std::vector<Video::Object *> &list, uint32_t firstStage, uint32_t *countList)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.set<UnorderedAccessView>(list);
                        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, UINT(list.size()), unorderedAccessCache.get(), countList);
                    }

                    void clearSamplerStateList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        samplerStateCache.clear(count);
                        d3dDeviceContext->PSSetSamplers(firstStage, count, samplerStateCache.get());
                    }

                    void clearConstantBufferList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        constantBufferCache.clear(count);
                        d3dDeviceContext->PSSetConstantBuffers(firstStage, count, constantBufferCache.get());
                    }

                    void clearResourceList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.clear(count);
                        d3dDeviceContext->PSSetShaderResources(firstStage, count, resourceCache.get());
                    }

                    void clearUnorderedAccessList(uint32_t count, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.clear(count);
                        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, count, unorderedAccessCache.get(), nullptr);
                    }
                };

            public:
                CComPtr<ID3D11DeviceContext> d3dDeviceContext;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;

            public:
                Context(ID3D11DeviceContext *d3dDeviceContext)
                    : d3dDeviceContext(d3dDeviceContext)
                    , computeSystemHandler(new ComputePipeline(d3dDeviceContext))
                    , vertexSystemHandler(new VertexPipeline(d3dDeviceContext))
                    , geomtrySystemHandler(new GeometryPipeline(d3dDeviceContext))
                    , pixelSystemHandler(new PixelPipeline(d3dDeviceContext))
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(computeSystemHandler);
                    GEK_REQUIRE(vertexSystemHandler);
                    GEK_REQUIRE(geomtrySystemHandler);
                    GEK_REQUIRE(pixelSystemHandler);
                }

                // Video::Context
                Pipeline * const computePipeline(void)
                {
                    GEK_REQUIRE(computeSystemHandler);

                    return computeSystemHandler.get();
                }

                Pipeline * const vertexPipeline(void)
                {
                    GEK_REQUIRE(vertexSystemHandler);

                    return vertexSystemHandler.get();
                }

                Pipeline * const geometryPipeline(void)
                {
                    GEK_REQUIRE(geomtrySystemHandler);

                    return geomtrySystemHandler.get();
                }

                Pipeline * const pixelPipeline(void)
                {
                    GEK_REQUIRE(pixelSystemHandler);

                    return pixelSystemHandler.get();
                }

                void generateMipMaps(Video::Texture *texture)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(texture);

                    d3dDeviceContext->GenerateMips(getObject<ShaderResourceView>(texture));
                }

                void resolveSamples(Video::Texture *destination, Video::Texture *source)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(destination);
                    GEK_REQUIRE(source);

                    d3dDeviceContext->ResolveSubresource(getObject<Resource>(destination), 0, getObject<Resource>(source), 0, DirectX::TextureFormatList[static_cast<uint8_t>(destination->getDescription().format)]);
                }

                void clearState(void)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearState();
                }

                void setViewportList(const std::vector<Video::ViewPort> &viewPortList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->RSSetViewports(UINT(viewPortList.size()), (D3D11_VIEWPORT *)viewPortList.data());
                }

                void setScissorList(const std::vector<Math::UInt4> &rectangleList)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->RSSetScissorRects(UINT(rectangleList.size()), (D3D11_RECT *)rectangleList.data());
                }

                void clearResource(Video::Object *object, const Math::Float4 &value)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(object);
                }

                void clearUnorderedAccess(Video::Object *object, const Math::Float4 &value)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(object);

                    d3dDeviceContext->ClearUnorderedAccessViewFloat(getObject<UnorderedAccessView>(object), value.data);
                }

                void clearUnorderedAccess(Video::Object *object, const uint32_t value[4])
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(object);

                    d3dDeviceContext->ClearUnorderedAccessViewUint(getObject<UnorderedAccessView>(object), value);
                }

                void clearRenderTarget(Video::Target *renderTarget, const Math::Float4 &clearColor)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(renderTarget);

                    d3dDeviceContext->ClearRenderTargetView(getObject<RenderTargetView>(renderTarget), clearColor.data);
                }

                void clearDepthStencilTarget(Video::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(depthBuffer);

                    d3dDeviceContext->ClearDepthStencilView(getObject<DepthTexture>(depthBuffer), 
                        ((flags & Video::ClearFlags::Depth ? D3D11_CLEAR_DEPTH : 0) |
                        (flags & Video::ClearFlags::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                        clearDepth, clearStencil);
                }

                void clearIndexBuffer(void)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
                }

                void clearVertexBufferList(uint32_t count, uint32_t firstSlot)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    vertexBufferCache.clear(count);
                    vertexBufferStrideCache.resize(count);
                    vertexBufferOffsetsCache.resize(count);
                    d3dDeviceContext->IASetVertexBuffers(firstSlot, count, vertexBufferCache.get(), vertexBufferStrideCache.data(), vertexBufferOffsetsCache.data());
                }

                void clearRenderTargetList(uint32_t count, bool depthBuffer)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    renderTargetViewCache.clear(count);
                    d3dDeviceContext->OMSetRenderTargets(count, renderTargetViewCache.get(), nullptr);
                }

                ObjectCache<ID3D11RenderTargetView> renderTargetViewCache;
                void setRenderTargetList(const std::vector<Video::Target *> &renderTargetList, Video::Object *depthBuffer)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    renderTargetViewCache.set<RenderTargetView>(renderTargetList);
                    d3dDeviceContext->OMSetRenderTargets(UINT(renderTargetList.size()), renderTargetViewCache.get(), getObject<DepthTexture>(depthBuffer));
                }

                void setRenderState(Video::Object *renderState)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(renderState);

                    d3dDeviceContext->RSSetState(getObject<RenderState>(renderState));
                }

                void setDepthState(Video::Object *depthState, uint32_t stencilReference)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(depthState);

                    d3dDeviceContext->OMSetDepthStencilState(getObject<DepthState>(depthState), stencilReference);
                }

                void setBlendState(Video::Object *blendState, const Math::Float4 &blendFactor, uint32_t mask)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(blendState);

                    d3dDeviceContext->OMSetBlendState(getObject<BlendState>(blendState), blendFactor.data, mask);
                }

                void setInputLayout(Video::Object *inputLayout)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->IASetInputLayout(getObject<InputLayout>(inputLayout));
                }

                void setIndexBuffer(Video::Buffer *indexBuffer, uint32_t offset)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(indexBuffer);

                    DXGI_FORMAT format = DirectX::BufferFormatList[static_cast<uint8_t>(indexBuffer->getDescription().format)];
                    d3dDeviceContext->IASetIndexBuffer(getObject<Buffer>(indexBuffer), format, offset);
                }

                ObjectCache<ID3D11Buffer> vertexBufferCache;
                std::vector<uint32_t> vertexBufferStrideCache;
                std::vector<uint32_t> vertexBufferOffsetsCache;
                void setVertexBufferList(const std::vector<Video::Buffer *> &vertexBufferList, uint32_t firstSlot, uint32_t *offsetList)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    
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

                void setPrimitiveType(Video::PrimitiveType primitiveType)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->IASetPrimitiveTopology(DirectX::TopologList[static_cast<uint8_t>(primitiveType)]);
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

                Video::ObjectPtr finishCommandList(void)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    CComPtr<ID3D11CommandList> d3dCommandList;
                    HRESULT resultValue = d3dDeviceContext->FinishCommandList(FALSE, &d3dCommandList);
                    if (FAILED(resultValue) || !d3dCommandList)
                    {
                        throw Video::OperationFailed("Unable to finish command list compilation");
                    }

                    return std::make_shared<CommandList>(d3dCommandList.p);
                }
            };

        public:
            HWND window = nullptr;
            bool isChildWindow = false;

            CComPtr<ID3D11Device> d3dDevice;
            CComPtr<ID3D11DeviceContext> d3dDeviceContext;
            CComPtr<IDXGISwapChain1> dxgiSwapChain;

            Video::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

        public:
            Device(Gek::Context *context, HWND window, Video::DeviceDescription deviceDescription)
                : ContextRegistration(context)
                , window(window)
                , isChildWindow(GetParent(window) != nullptr)
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
                    throw Video::FeatureLevelNotSupported("Direct3D 11.0 feature level required");
                }

                if (FAILED(resultValue) || !d3dDevice || !d3dDeviceContext)
                {
                    throw Video::InitializationFailed("Unable to create rendering device and context");
                }

                CComPtr<IDXGIFactory2> dxgiFactory;
                resultValue = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
                if (FAILED(resultValue) || !dxgiFactory)
                {
                    throw Video::InitializationFailed("Unable to get graphics factory");
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
                resultValue = dxgiFactory->CreateSwapChainForHwnd(d3dDevice, window, &swapChainDescription, nullptr, nullptr, &dxgiSwapChain);
                if (FAILED(resultValue) || !dxgiSwapChain)
                {
                    throw Video::InitializationFailed("Unable to create swap chain for window");
                }

                dxgiFactory->MakeWindowAssociation(window, 0);

#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                CComQIPtr<ID3D11InfoQueue> d3dInfoQueue(d3dDebug);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                //d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif

                defaultContext = std::make_shared<Context>(d3dDeviceContext);
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

            // Video::Debug::Device
            void * getDevice(void)
            {
                return d3dDevice.p;
            }

            // Video::Device
            Video::DisplayModeList getDisplayModeList(Video::Format format) const
            {
                Video::DisplayModeList displayModeList;

                CComPtr<IDXGIOutput> dxgiOutput;
                dxgiSwapChain->GetContainingOutput(&dxgiOutput);
                if (dxgiOutput)
                {
                    uint32_t modeCount = 0;
                    dxgiOutput->GetDisplayModeList(DirectX::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, nullptr);

                    std::vector<DXGI_MODE_DESC> dxgiDisplayModeList(modeCount);
                    dxgiOutput->GetDisplayModeList(DirectX::TextureFormatList[static_cast<uint8_t>(format)], 0, &modeCount, dxgiDisplayModeList.data());

                    for (auto &dxgiDisplayMode : dxgiDisplayModeList)
                    {
                        if (dxgiDisplayMode.ScanlineOrdering == DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE && dxgiDisplayMode.Scaling == DXGI_MODE_SCALING_CENTERED)
                        {
                            auto getAspectRatio = [](uint32_t width, uint32_t height) -> Video::DisplayMode::AspectRatio
                            {
                                const float AspectRatio4x3 = (4.0f / 3.0f);
                                const float AspectRatio16x9 = (16.0f / 9.0f);
                                const float AspectRatio16x10 = (16.0f / 10.0f);
                                float aspectRatio = (float(width) / float(height));
                                if (std::abs(aspectRatio - AspectRatio4x3) < Math::Epsilon)
                                {
                                    return Video::DisplayMode::AspectRatio::_4x3;
                                }
                                else if (std::abs(aspectRatio - AspectRatio16x9) < Math::Epsilon)
                                {
                                    return Video::DisplayMode::AspectRatio::_16x9;
                                }
                                else if (std::abs(aspectRatio - AspectRatio16x10) < Math::Epsilon)
                                {
                                    return Video::DisplayMode::AspectRatio::_16x10;
                                }
                                else
                                {
                                    return Video::DisplayMode::AspectRatio::Unknown;
                                }
                            };

                            Video::DisplayMode displayMode;
                            displayMode.width = dxgiDisplayMode.Width;
                            displayMode.height = dxgiDisplayMode.Height;
                            displayMode.format = DirectX::getFormat(dxgiDisplayMode.Format);
                            displayMode.aspectRatio = getAspectRatio(displayMode.width, displayMode.height);
                            displayMode.refreshRate.numerator = dxgiDisplayMode.RefreshRate.Numerator;
                            displayMode.refreshRate.denominator = dxgiDisplayMode.RefreshRate.Denominator;
                            displayModeList.push_back(displayMode);
                        }
                    }

                    concurrency::parallel_sort(std::begin(displayModeList), std::end(displayModeList), [](const Video::DisplayMode &left, const Video::DisplayMode &right) -> bool
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

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;

                HRESULT resultValue = dxgiSwapChain->SetFullscreenState(fullScreen, nullptr);
                if (FAILED(resultValue))
                {
                    if (fullScreen)
                    {
                        throw Video::OperationFailed("Unablet to set fullscreen state");
                    }
                    else
                    {
                        throw Video::OperationFailed("Unablet to set windowed state");
                    }
                }
            }

            void setDisplayMode(const Video::DisplayMode &displayMode)
            {
                GEK_REQUIRE(dxgiSwapChain);

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;

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
                    throw Video::OperationFailed("Unable to set display mode");
                }
            }

            void handleResize(void)
            {
                GEK_REQUIRE(dxgiSwapChain);

                d3dDeviceContext->ClearState();
                backBuffer = nullptr;
                
                HRESULT resultValue = dxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
                if (FAILED(resultValue))
                {
                    throw Video::OperationFailed("Unable to resize swap chain buffers to window size");
                }
            }

            const char * const getSemanticMoniker(Video::InputElement::Semantic semantic)
            {
                return DirectX::SemanticNameList[static_cast<uint8_t>(semantic)];
            }

            Video::Target * const getBackBuffer(void)
            {
                if (!backBuffer)
                {
                    CComPtr<ID3D11Texture2D> d3dRenderTarget;
                    HRESULT resultValue = dxgiSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                    if (FAILED(resultValue) || !d3dRenderTarget)
                    {
                        throw Video::OperationFailed("Unable to get swap chain primary buffer");
                    }

                    CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                    resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                    if (FAILED(resultValue) || !d3dRenderTargetView)
                    {
                        throw Video::OperationFailed("Unable to create render target view for back buffer");
                    }

                    D3D11_TEXTURE2D_DESC textureDescription;
                    d3dRenderTarget->GetDesc(&textureDescription);

                    Video::TextureDescription description;
                    description.width = textureDescription.Width;
                    description.height = textureDescription.Height;
                    description.format = DirectX::getFormat(textureDescription.Format);
                    backBuffer = std::make_shared<TargetTexture>(d3dRenderTarget.p, d3dRenderTargetView.p, description);
                }

                return backBuffer.get();
            }

            Video::Device::Context * const getDefaultContext(void)
            {
                GEK_REQUIRE(defaultContext);

                return defaultContext.get();
            }

            Video::Device::ContextPtr createDeferredContext(void)
            {
                GEK_REQUIRE(d3dDevice);

                CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
                HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext);
                if (FAILED(resultValue) || !d3dDeferredDeviceContext)
                {
                    throw Video::OperationFailed("Unable to create deferred context");
                }

                return std::make_shared<Context>(d3dDeferredDeviceContext.p);
            }

            Video::ObjectPtr createEvent(void)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_QUERY_DESC description;
                description.Query = D3D11_QUERY_EVENT;
                description.MiscFlags = 0;

                CComPtr<ID3D11Query> d3dQuery;
                HRESULT resultValue = d3dDevice->CreateQuery(&description, &d3dQuery);
                if (FAILED(resultValue) || !d3dQuery)
                {
                    throw Video::OperationFailed("Unable to create event");
                }

                return std::make_shared<Event>(d3dQuery);
            }

            void setEvent(Video::Object *gpuEvent)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(gpuEvent);

                d3dDeviceContext->End(getObject<Event>(gpuEvent));
            }

            bool isEventSet(Video::Object *gpuEvent)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(gpuEvent);

                uint32_t isEventSet = 0;
                if (FAILED(d3dDeviceContext->GetData(getObject<Event>(gpuEvent), (void *)&isEventSet, sizeof(uint32_t), TRUE)))
                {
                    isEventSet = 0;
                }

                return (isEventSet == 1);
            }

            Video::ObjectPtr createRenderState(const Video::RenderStateInformation &renderState)
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
                rasterizerDescription.FillMode = DirectX::FillModeList[static_cast<uint8_t>(renderState.fillMode)];
                rasterizerDescription.CullMode = DirectX::CullModeList[static_cast<uint8_t>(renderState.cullMode)];

                CComPtr<ID3D11RasterizerState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateRasterizerState(&rasterizerDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    throw Video::CreateObjectFailed("Unable to create rasterizer state");
                }

                return std::make_shared<RenderState>(d3dStates);
            }

            Video::ObjectPtr createDepthState(const Video::DepthStateInformation &depthState)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_DEPTH_STENCIL_DESC depthStencilDescription;
                depthStencilDescription.DepthEnable = depthState.enable;
                depthStencilDescription.DepthFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthState.comparisonFunction)];
                depthStencilDescription.StencilEnable = depthState.stencilEnable;
                depthStencilDescription.StencilReadMask = depthState.stencilReadMask;
                depthStencilDescription.StencilWriteMask = depthState.stencilWriteMask;
                depthStencilDescription.FrontFace.StencilFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilFrontState.failOperation)];
                depthStencilDescription.FrontFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilFrontState.depthFailOperation)];
                depthStencilDescription.FrontFace.StencilPassOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilFrontState.passOperation)];
                depthStencilDescription.FrontFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthState.stencilFrontState.comparisonFunction)];
                depthStencilDescription.BackFace.StencilFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilBackState.failOperation)];
                depthStencilDescription.BackFace.StencilDepthFailOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilBackState.depthFailOperation)];
                depthStencilDescription.BackFace.StencilPassOp = DirectX::StencilOperationList[static_cast<uint8_t>(depthState.stencilBackState.passOperation)];
                depthStencilDescription.BackFace.StencilFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(depthState.stencilBackState.comparisonFunction)];
                depthStencilDescription.DepthWriteMask = DirectX::DepthWriteMaskList[static_cast<uint8_t>(depthState.writeMask)];

                CComPtr<ID3D11DepthStencilState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateDepthStencilState(&depthStencilDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    throw Video::CreateObjectFailed("Unable to create depth stencil state");
                }

                return std::make_shared<DepthState>(d3dStates);
            }

            Video::ObjectPtr createBlendState(const Video::UnifiedBlendStateInformation &blendState)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendState.alphaToCoverage;
                blendDescription.IndependentBlendEnable = false;
                blendDescription.RenderTarget[0].BlendEnable = blendState.enable;
                blendDescription.RenderTarget[0].SrcBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.colorSource)];
                blendDescription.RenderTarget[0].DestBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.colorDestination)];
                blendDescription.RenderTarget[0].BlendOp = DirectX::BlendOperationList[static_cast<uint8_t>(blendState.colorOperation)];
                blendDescription.RenderTarget[0].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.alphaSource)];
                blendDescription.RenderTarget[0].DestBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.alphaDestination)];
                blendDescription.RenderTarget[0].BlendOpAlpha = DirectX::BlendOperationList[static_cast<uint8_t>(blendState.alphaOperation)];
                blendDescription.RenderTarget[0].RenderTargetWriteMask = 0;
                if (blendState.writeMask & Video::BlendStateInformation::Mask::R)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                }

                if (blendState.writeMask & Video::BlendStateInformation::Mask::G)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                }

                if (blendState.writeMask & Video::BlendStateInformation::Mask::B)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                }

                if (blendState.writeMask & Video::BlendStateInformation::Mask::A)
                {
                    blendDescription.RenderTarget[0].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                }

                CComPtr<ID3D11BlendState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    throw Video::CreateObjectFailed("Unable to create unified blend state");
                }

                return std::make_shared<BlendState>(d3dStates);
            }

            Video::ObjectPtr createBlendState(const Video::IndependentBlendStateInformation &blendState)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_BLEND_DESC blendDescription;
                blendDescription.AlphaToCoverageEnable = blendState.alphaToCoverage;
                blendDescription.IndependentBlendEnable = true;
                for (uint32_t renderTarget = 0; renderTarget < 8; ++renderTarget)
                {
                    blendDescription.RenderTarget[renderTarget].BlendEnable = blendState.targetStates[renderTarget].enable;
                    blendDescription.RenderTarget[renderTarget].SrcBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.targetStates[renderTarget].colorSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlend = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.targetStates[renderTarget].colorDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOp = DirectX::BlendOperationList[static_cast<uint8_t>(blendState.targetStates[renderTarget].colorOperation)];
                    blendDescription.RenderTarget[renderTarget].SrcBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaSource)];
                    blendDescription.RenderTarget[renderTarget].DestBlendAlpha = DirectX::BlendSourceList[static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaDestination)];
                    blendDescription.RenderTarget[renderTarget].BlendOpAlpha = DirectX::BlendOperationList[static_cast<uint8_t>(blendState.targetStates[renderTarget].alphaOperation)];
                    blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask = 0;
                    if (blendState.targetStates[renderTarget].writeMask & Video::BlendStateInformation::Mask::R)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_RED;
                    }

                    if (blendState.targetStates[renderTarget].writeMask & Video::BlendStateInformation::Mask::G)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_GREEN;
                    }

                    if (blendState.targetStates[renderTarget].writeMask & Video::BlendStateInformation::Mask::B)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_BLUE;
                    }

                    if (blendState.targetStates[renderTarget].writeMask & Video::BlendStateInformation::Mask::A)
                    {
                        blendDescription.RenderTarget[renderTarget].RenderTargetWriteMask |= D3D10_COLOR_WRITE_ENABLE_ALPHA;
                    }
                }

                CComPtr<ID3D11BlendState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateBlendState(&blendDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    throw Video::CreateObjectFailed("Unable to create independent blend state");
                }

                return std::make_shared<BlendState>(d3dStates);
            }

            Video::ObjectPtr createSamplerState(const Video::SamplerStateInformation &samplerState)
            {
                GEK_REQUIRE(d3dDevice);

                D3D11_SAMPLER_DESC samplerDescription;
                samplerDescription.AddressU = DirectX::AddressModeList[static_cast<uint8_t>(samplerState.addressModeU)];
                samplerDescription.AddressV = DirectX::AddressModeList[static_cast<uint8_t>(samplerState.addressModeV)];
                samplerDescription.AddressW = DirectX::AddressModeList[static_cast<uint8_t>(samplerState.addressModeW)];
                samplerDescription.MipLODBias = samplerState.mipLevelBias;
                samplerDescription.MaxAnisotropy = samplerState.maximumAnisotropy;
                samplerDescription.ComparisonFunc = DirectX::ComparisonFunctionList[static_cast<uint8_t>(samplerState.comparisonFunction)];
                samplerDescription.BorderColor[0] = samplerState.borderColor.r;
                samplerDescription.BorderColor[1] = samplerState.borderColor.g;
                samplerDescription.BorderColor[2] = samplerState.borderColor.b;
                samplerDescription.BorderColor[3] = samplerState.borderColor.a;
                samplerDescription.MinLOD = samplerState.minimumMipLevel;
                samplerDescription.MaxLOD = samplerState.maximumMipLevel;
                samplerDescription.Filter = DirectX::FilterList[static_cast<uint8_t>(samplerState.filterMode)];

                CComPtr<ID3D11SamplerState> d3dStates;
                HRESULT resultValue = d3dDevice->CreateSamplerState(&samplerDescription, &d3dStates);
                if (FAILED(resultValue) || !d3dStates)
                {
                    throw Video::CreateObjectFailed("Unable to create sampler state");
                }

                return std::make_shared<SamplerState>(d3dStates);
            }

            Video::BufferPtr createBuffer(const Video::BufferDescription &description, const void *data)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(description.count > 0);

                uint32_t stride = description.stride;
                if (description.format != Video::Format::Unknown)
                {
                    if (description.stride > 0)
                    {
                        throw Video::InvalidParameter("Buffer requires only a format or an element stride");
                    }

                    stride = DirectX::FormatStrideList[static_cast<uint8_t>(description.format)];
                }
                else if (description.stride > 0)
                {
                }
                else
                {
                    throw Video::InvalidParameter("Buffer requires either a format or an element stride");
                }

                D3D11_BUFFER_DESC bufferDescription;
                bufferDescription.ByteWidth = (stride * description.count);
                switch (description.type)
                {
                case Video::BufferDescription::Type::Structured:
                    bufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
                    bufferDescription.StructureByteStride = stride;
                    bufferDescription.BindFlags = 0;
                    break;

                default:
                    bufferDescription.MiscFlags = 0;
                    bufferDescription.StructureByteStride = 0;
                    switch (description.type)
                    {
                    case Video::BufferDescription::Type::Vertex:
                        bufferDescription.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                        break;

                    case Video::BufferDescription::Type::Index:
                        bufferDescription.BindFlags = D3D11_BIND_INDEX_BUFFER;
                        break;

                    case Video::BufferDescription::Type::Constant:
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
                else if (description.flags & Video::BufferDescription::Flags::Staging)
                {
                    bufferDescription.Usage = D3D11_USAGE_STAGING;
                    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
                }
                else if (description.flags & Video::BufferDescription::Flags::Mappable)
                {
                    bufferDescription.Usage = D3D11_USAGE_DYNAMIC;
                    bufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                }
                else
                {
                    bufferDescription.Usage = D3D11_USAGE_DEFAULT;
                    bufferDescription.CPUAccessFlags = 0;
                }

                if (description.flags & Video::BufferDescription::Flags::Resource)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (description.flags & Video::BufferDescription::Flags::UnorderedAccess)
                {
                    bufferDescription.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
                }

                CComPtr<ID3D11Buffer> d3dBuffer;
                if (data == nullptr)
                {
                    HRESULT resultValue = d3dDevice->CreateBuffer(&bufferDescription, nullptr, &d3dBuffer);
                    if (FAILED(resultValue) || !d3dBuffer)
                    {
                        throw Video::CreateObjectFailed("Unable to dynamic buffer");
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
                        throw Video::CreateObjectFailed("Unable to create static buffer");
                    }
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Video::BufferDescription::Flags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;
                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        throw Video::CreateObjectFailed("Unable to create buffer shader resource view");
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Video::BufferDescription::Flags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(description.format)];
                    viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = description.count;
                    viewDescription.Buffer.Flags = (description.flags & Video::BufferDescription::Flags::Counter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0);

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        throw Video::CreateObjectFailed("Unable to create buffer unordered access view");
                    }
                }

                return std::make_shared<Buffer>(d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView, description);
            }

            void mapBuffer(Video::Buffer *buffer, void *&data, Video::Map mapping)
            {
                GEK_REQUIRE(d3dDeviceContext);

                D3D11_MAP d3dMapping = DirectX::MapList[static_cast<uint8_t>(mapping)];

                D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                mappedSubResource.pData = nullptr;
                mappedSubResource.RowPitch = 0;
                mappedSubResource.DepthPitch = 0;

                HRESULT resultValue = d3dDeviceContext->Map(getObject<Buffer>(buffer), 0, d3dMapping, 0, &mappedSubResource);
                if (FAILED(resultValue))
                {
                    throw Video::OperationFailed("Unable to map buffer to system memory");
                }

                data = mappedSubResource.pData;
            }

            void unmapBuffer(Video::Buffer *buffer)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(buffer);

                d3dDeviceContext->Unmap(getObject<Buffer>(buffer), 0);
            }

            void updateResource(Video::Object *object, const void *data)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(object);
                GEK_REQUIRE(data);

                d3dDeviceContext->UpdateSubresource(getObject<Resource>(object), 0, nullptr, data, 0, 0);
            }

            void copyResource(Video::Object *destination, Video::Object *source)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(destination);
                GEK_REQUIRE(source);

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

                d3dDeviceContext->CopyResource(getObject<Resource>(destination), getObject<Resource>(source));
            }

            Video::ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize)
            {
                GEK_REQUIRE(compiledSize);
                GEK_REQUIRE(compiledData);

                std::vector<D3D11_INPUT_ELEMENT_DESC> d3dElementList;
                Video::InputElement::Source lastSource = Video::InputElement::Source::Vertex;
                uint32_t semanticIndexList[static_cast<uint8_t>(Video::InputElement::Semantic::Count)] = { 0 };
                for (auto &element : elementList)
                {
                    D3D11_INPUT_ELEMENT_DESC elementDesc;
                    if (lastSource != element.source)
                    {
                        elementDesc.AlignedByteOffset = 0;
                    }
                    else
                    {
                        elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
                    }

                    lastSource = element.source;
                    elementDesc.Format = DirectX::BufferFormatList[static_cast<uint8_t>(element.format)];
                    elementDesc.SemanticName = DirectX::SemanticNameList[static_cast<uint8_t>(element.semantic)];
                    elementDesc.SemanticIndex = semanticIndexList[static_cast<uint8_t>(element.semantic)]++;
                    elementDesc.InputSlot = element.sourceIndex;
                    switch (element.source)
                    {
                    case Video::InputElement::Source::Instance:
                        elementDesc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
                        elementDesc.InstanceDataStepRate = 1;
                        break;

                    case Video::InputElement::Source::Vertex:
                    default:
                        elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                        elementDesc.InstanceDataStepRate = 0;
                        break;
                    };

                    d3dElementList.push_back(elementDesc);
                }

                CComPtr<ID3D11InputLayout> d3dInputLayout;
                HRESULT resultValue = d3dDevice->CreateInputLayout(d3dElementList.data(), UINT(d3dElementList.size()), compiledData, compiledSize, &d3dInputLayout);
                if (FAILED(resultValue) || !d3dInputLayout)
                {
                    throw Video::CreateObjectFailed("Unable to create input vertex layout");
                }

                return std::make_shared<InputLayout>(d3dInputLayout);
            }

            template <class SHADER, class PROGRAM, typename RETURN, typename CLASS, typename... PARAMETERS>
            Video::ObjectPtr createProgram(const void *compiledData, uint32_t compiledSize, RETURN(__stdcall CLASS::*function)(PARAMETERS...))
            {
                GEK_REQUIRE(compiledData);
                GEK_REQUIRE(compiledSize);
                GEK_REQUIRE(function);

                CComPtr<SHADER> d3dShader;
                HRESULT resultValue = (d3dDevice->*function)(compiledData, compiledSize, nullptr, &d3dShader);
                if (FAILED(resultValue) || !d3dShader)
                {
                    throw Video::CreateObjectFailed("Unable to create program from compiled data");
                }

                return std::make_shared<PROGRAM>(d3dShader);
            }

            Video::ObjectPtr createProgram(Video::PipelineType pipelineType, const void *compiledData, uint32_t compiledSize)
            {
                GEK_REQUIRE(compiledData);
                GEK_REQUIRE(compiledSize);

                switch (pipelineType)
                {
                case Video::PipelineType::Compute:
                    return createProgram<ID3D11ComputeShader, ComputeProgram>(compiledData, compiledSize, &ID3D11Device::CreateComputeShader);

                case Video::PipelineType::Vertex:
                    return createProgram<ID3D11VertexShader, VertexProgram>(compiledData, compiledSize, &ID3D11Device::CreateVertexShader);

                case Video::PipelineType::Geometry:
                    return createProgram<ID3D11GeometryShader, GeometryProgram>(compiledData, compiledSize, &ID3D11Device::CreateGeometryShader);

                case Video::PipelineType::Pixel:
                    return createProgram<ID3D11PixelShader, PixelProgram>(compiledData, compiledSize, &ID3D11Device::CreatePixelShader);
                };

                throw Video::CreateObjectFailed("Unknown program pipline encountered");
            }

            std::vector<uint8_t> compileProgram(const StringUTF8 &name, const StringUTF8 &type, const StringUTF8 &uncompiledProgram, const StringUTF8 &entryFunction)
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

                CComPtr<ID3DBlob> d3dShaderBlob;
                CComPtr<ID3DBlob> d3dCompilerErrors;
                HRESULT resultValue = D3DCompile(uncompiledProgram, (uncompiledProgram.size() + 1), name, nullptr, nullptr, entryFunction, type, flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
                if (FAILED(resultValue) || !d3dShaderBlob)
                {
                    OutputDebugStringW(String::Format(L"D3DCompile Failed: %v\r\n%v\r\n", resultValue, (const char *)d3dCompilerErrors->GetBufferPointer()));
                    throw Video::ProgramCompilationFailed("Unable to compile program");
                }

                uint8_t *data = (uint8_t *)d3dShaderBlob->GetBufferPointer();
                return std::vector<uint8_t>(data, (data + d3dShaderBlob->GetBufferSize()));
            }

            std::vector<uint8_t> compileProgram(Video::PipelineType pipelineType, const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction)
            {
                GEK_REQUIRE(name);
                GEK_REQUIRE(uncompiledProgram);
                GEK_REQUIRE(entryFunction);

                switch (pipelineType)
                {
                case Video::PipelineType::Compute:
                    return compileProgram(name, "cs_5_0", uncompiledProgram, entryFunction);

                case Video::PipelineType::Vertex:
                    return compileProgram(name, "vs_5_0", uncompiledProgram, entryFunction);

                case Video::PipelineType::Geometry:
                    return compileProgram(name, "gs_5_0", uncompiledProgram, entryFunction);

                case Video::PipelineType::Pixel:
                    return compileProgram(name, "ps_5_0", uncompiledProgram, entryFunction);
                };

                throw Video::CreateObjectFailed("Unknown program pipline encountered");
            }

            Video::TexturePtr createTexture(const Video::TextureDescription &description, const void *data)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(description.format != Video::Format::Unknown);
                GEK_REQUIRE(description.width != 0);
                GEK_REQUIRE(description.height != 0);
                GEK_REQUIRE(description.depth != 0);

                uint32_t bindFlags = 0;
                if (description.flags & Video::TextureDescription::Flags::RenderTarget)
                {
                    if (description.flags & Video::TextureDescription::Flags::DepthTarget)
                    {
                        throw Video::InvalidParameter("Cannot create render target when depth target flag also specified");
                    }

                    bindFlags |= D3D11_BIND_RENDER_TARGET;
                }

                if (description.flags & Video::TextureDescription::Flags::DepthTarget)
                {
                    if (description.depth > 1)
                    {
                        throw Video::InvalidParameter("Depth target must have depth of one");
                    }

                    bindFlags |= D3D11_BIND_DEPTH_STENCIL;
                }

                if (description.flags & Video::TextureDescription::Flags::Resource)
                {
                    bindFlags |= D3D11_BIND_SHADER_RESOURCE;
                }

                if (description.flags & Video::TextureDescription::Flags::UnorderedAccess)
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
                        throw Video::CreateObjectFailed("Unable to create 2D texture");
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
                        throw Video::CreateObjectFailed("Unable to create 3D texture");
                    }

                    d3dResource = texture3D;
                }

                if (!d3dResource)
                {
                    throw Video::CreateObjectFailed("Unable to get texture resource");
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (description.flags & Video::TextureDescription::Flags::Resource)
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

                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dResource, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        throw Video::CreateObjectFailed("Unable to create texture shader resource view");
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (description.flags & Video::TextureDescription::Flags::UnorderedAccess)
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

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dResource, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        throw Video::CreateObjectFailed("Unable to create texture unordered access view");
                    }
                }

                if (description.flags & Video::TextureDescription::Flags::RenderTarget)
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
                        throw Video::CreateObjectFailed("Unable to create render target view");
                    }

                    return std::make_shared<TargetViewTexture>(d3dResource.p, d3dRenderTargetView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, description);
                }
                else if (description.flags & Video::TextureDescription::Flags::DepthTarget)
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
                        throw Video::CreateObjectFailed("Unable to create depth stencil view");
                    }

                    return std::make_shared<DepthTexture>(d3dResource.p, d3dDepthStencilView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, description);
                }
                else
                {
                    return std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, description);
                }
            }

            Video::TexturePtr loadTexture(const wchar_t *fileName, uint32_t flags)
            {
                GEK_REQUIRE(d3dDevice);

                std::vector<uint8_t> buffer;
                FileSystem::Load(fileName, buffer);

                String extension(FileSystem::GetExtension(fileName));
                std::function<HRESULT(const std::vector<uint8_t> &, ::DirectX::ScratchImage &)> load;
                if (extension.compareNoCase(L".dds") == 0)
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromDDSMemory(buffer.data(), buffer.size(), 0, nullptr, image); };
                }
                else if (extension.compareNoCase(L".tga") == 0)
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromTGAMemory(buffer.data(), buffer.size(), nullptr, image); };
                }
                else if (extension.compareNoCase(L".png") == 0)
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_PNG, nullptr, image); };
                }
                else if (extension.compareNoCase(L".bmp") == 0)
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_BMP, nullptr, image); };
                }
                else if (extension.compareNoCase(L".jpg") == 0 ||
                    extension.compareNoCase(L".jpeg") == 0)
                {
                    load = [](const std::vector<uint8_t> &buffer, ::DirectX::ScratchImage &image) -> HRESULT { return ::DirectX::LoadFromWICMemory(buffer.data(), buffer.size(), ::DirectX::WIC_CODEC_JPEG, nullptr, image); };
                }

                if (!load)
                {
                    throw Video::InvalidFileType("Unknown texture extension encountered");
                }

                ::DirectX::ScratchImage image;
                HRESULT resultValue = load(buffer, image);
                if (FAILED(resultValue))
                {
                    throw Video::LoadFileFailed("Unable to load texture from file");
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                resultValue = ::DirectX::CreateShaderResourceViewEx(d3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, (flags & Video::TextureLoadFlags::sRGB), &d3dShaderResourceView);
                if (FAILED(resultValue) || !d3dShaderResourceView)
                {
                    throw Video::CreateObjectFailed("Unable to create texture shader resource view");
                }

                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                if (FAILED(resultValue) || !d3dResource)
                {
                    throw Video::CreateObjectFailed("Unable to get texture resource");
                }

                Video::TextureDescription description;
                description.width = image.GetMetadata().width;
                description.height = image.GetMetadata().height;
                description.depth = image.GetMetadata().depth;
                description.format = DirectX::getFormat(image.GetMetadata().format);
                description.mipMapCount = image.GetMetadata().mipLevels;
                return std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, nullptr, description);
            }

            void executeCommandList(Video::Object *commandList)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(commandList);

                CComQIPtr<ID3D11CommandList> d3dCommandList;
                d3dDeviceContext->ExecuteCommandList(getObject<CommandList>(commandList), FALSE);
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
