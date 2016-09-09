#pragma warning(disable : 4005)

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\VideoDevice.h"
#include <atlbase.h>
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
	// Function Traits
	// http://stackoverflow.com/questions/2562320/specializing-a-template-on-a-lambda-in-c0x

	template <typename CLASS>
    struct FunctionCache
    {
        using ReturnType = typename FunctionCache<decltype(&CLASS::operator())>::ReturnType;
        using ArgumentTypes = typename FunctionCache<decltype(&CLASS::operator())>::ArgumentTypes;
    };

    template <typename RETURN, typename CLASS, typename... ARGUMENTS>
    struct FunctionCache<RETURN(__stdcall CLASS::*)(ARGUMENTS...)>
    {
        using ReturnType = RETURN;
        using ArgumentTypes = std::tuple<typename std::decay<ARGUMENTS>::type...>;

        ArgumentTypes cache;
        void operator()(CLASS *classObject, RETURN(__stdcall CLASS::*function)(ARGUMENTS...), ARGUMENTS... arguments)
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
            (sizeof(uint8_t)  * 4), // DXGI_FORMAT_R8G8B8A8_UINT,
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
    }; // namespace DirectX
	
	namespace Direct3D11
    {
		template <typename CLASS>
        void setDebugName(CComPtr<CLASS> &object, const wchar_t *name)
        {
            if (object)
            {
                StringUTF8 nameUTF8(name);
                object->SetPrivateData(WKPDID_D3DDebugObjectName, nameUTF8.length(), nameUTF8.c_str());
            }
        }

        class CommandList
            : public Video::Object
        {
        public:
            CComPtr<ID3D11CommandList> d3dCommandList;

        public:
            CommandList(ID3D11CommandList *d3dCommandList)
                : d3dCommandList(d3dCommandList)
            {
                GEK_REQUIRE(d3dCommandList);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dCommandList, name);
            }
        };

        class RenderState
            : public Video::Object
        {
        public:
            CComQIPtr<ID3D11RasterizerState> d3dRenderState;

        public:
            RenderState(ID3D11RasterizerState *d3dRenderState)
                : d3dRenderState(d3dRenderState)
            {
                GEK_REQUIRE(d3dRenderState);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dRenderState, name);
            }
        };

        class DepthState
            : public Video::Object
        {
        public:
            CComQIPtr<ID3D11DepthStencilState> d3dDepthState;

        public:
            DepthState(ID3D11DepthStencilState *d3dDepthState)
                : d3dDepthState(d3dDepthState)
            {
                GEK_REQUIRE(d3dDepthState);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dDepthState, name);
            }
        };

        class BlendState
            : public Video::Object
        {
        public:
            CComQIPtr<ID3D11BlendState> d3dBlendState;

        public:
            BlendState(ID3D11BlendState *d3dBlendState)
                : d3dBlendState(d3dBlendState)
            {
                GEK_REQUIRE(d3dBlendState);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dBlendState, name);
            }
        };

        class SamplerState
            : public Video::Object
        {
        public:
            CComQIPtr<ID3D11SamplerState> d3dSamplerState;

        public:
            SamplerState(ID3D11SamplerState *d3dSamplerState)
                : d3dSamplerState(d3dSamplerState)
            {
                GEK_REQUIRE(d3dSamplerState);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dSamplerState, name);
            }
        };

        class Event
            : public Video::Object
        {
        public:
            CComPtr<ID3D11Query> d3dQuery;

        public:
            Event(ID3D11Query *d3dQuery)
                : d3dQuery(d3dQuery)
            {
                GEK_REQUIRE(d3dQuery);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dQuery, name);
            }
        };

		class InputLayout
			: public Video::Object
		{
		public:
			CComPtr<ID3D11InputLayout> d3dInputLayout;

		public:
			InputLayout(ID3D11InputLayout *d3dInputLayout)
				: d3dInputLayout(d3dInputLayout)
			{
				GEK_REQUIRE(d3dInputLayout);
			}

			void setName(const wchar_t *name)
			{
				setDebugName(d3dInputLayout, name);
			}
		};

		class ComputeProgram
            : public Video::Object
        {
        public:
            CComPtr<ID3D11ComputeShader> d3dShader;

        public:
            ComputeProgram(ID3D11ComputeShader *d3dShader)
                : d3dShader(d3dShader)
            {
                GEK_REQUIRE(d3dShader);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dShader, name);
            }
        };

        class VertexProgram
            : public Video::Object
        {
        public:
            CComPtr<ID3D11VertexShader> d3dShader;

        public:
            VertexProgram(ID3D11VertexShader *d3dShader)
                : d3dShader(d3dShader)
            {
                GEK_REQUIRE(d3dShader);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dShader, name);
            }
        };

        class GeometryProgram
            : public Video::Object
        {
        public:
            CComPtr<ID3D11GeometryShader> d3dShader;

        public:
            GeometryProgram(ID3D11GeometryShader *d3dShader)
                : d3dShader(d3dShader)
            {
                GEK_REQUIRE(d3dShader);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dShader, name);
            }
        };

        class PixelProgram
            : public Video::Object
        {
        public:
            CComPtr<ID3D11PixelShader> d3dShader;

        public:
            PixelProgram(ID3D11PixelShader *d3dShader)
                : d3dShader(d3dShader)
            {
                GEK_REQUIRE(d3dShader);
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dShader, name);
            }
        };

        class Resource
        {
        public:
            CComQIPtr<ID3D11Resource> d3dResource;

        public:
            template <typename TYPE>
            Resource(TYPE *d3dResource)
                : d3dResource(d3dResource)
            {
                GEK_REQUIRE(d3dResource);
            }

            virtual ~Resource(void) = default;
        };

        class ShaderResourceView
        {
        public:
            CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;

        public:
            ShaderResourceView(ID3D11ShaderResourceView *d3dShaderResourceView)
                : d3dShaderResourceView(d3dShaderResourceView)
            {
            }

            virtual ~ShaderResourceView(void) = default;
        };

        class UnorderedAccessView
        {
        public:
            CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;

        public:
            UnorderedAccessView(ID3D11UnorderedAccessView *d3dUnorderedAccessView)
                : d3dUnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            virtual ~UnorderedAccessView(void) = default;
        };

        class RenderTargetView
        {
        public:
            CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;

        public:
            RenderTargetView(ID3D11RenderTargetView *d3dRenderTargetView)
                : d3dRenderTargetView(d3dRenderTargetView)
            {
            }

            virtual ~RenderTargetView(void) = default;
        };

        class Buffer
            : public Video::Buffer
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            CComPtr<ID3D11Buffer> d3dBuffer;
            Video::Format format;
            uint32_t stride;
            uint32_t count;

        public:
            Buffer(ID3D11Buffer *d3dBuffer, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, uint32_t stride, uint32_t count)
                : Resource(d3dBuffer)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
                , d3dBuffer(d3dBuffer)
                , format(format)
                , stride(stride)
                , count(count)
            {
            }

            virtual ~Buffer(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(d3dResource, name);
                setDebugName(d3dBuffer, name);
                setDebugName(d3dShaderResourceView, name);
                setDebugName(d3dUnorderedAccessView, name);
            }

            // Video::Buffer
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getStride(void)
            {
                return stride;
            }

            uint32_t getCount(void)
            {
                return stride;
            }
        };

        class Texture
            : virtual public Video::Texture
        {
        public:
            Video::Format format;
            uint32_t width;
            uint32_t height;
            uint32_t depth;

        public:
            Texture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : format(format)
                , width(width)
                , height(height)
                , depth(depth)
            {
            }

            virtual ~Texture(void) = default;

            // Video::Texture
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getWidth(void)
            {
                return width;
            }

            uint32_t getHeight(void)
            {
                return height;
            }

            uint32_t getDepth(void)
            {
                return depth;
            }
        };

        class ViewTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            ViewTexture(ID3D11Resource *d3dResource, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Texture(format, width, height, depth)
                , Resource(d3dResource)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            void setName(const wchar_t *name)
            {
                setDebugName(d3dResource, name);
                setDebugName(d3dShaderResourceView, name);
                setDebugName(d3dUnorderedAccessView, name);
            }
        };

        class Target
            : virtual public Video::Target
        {
        public:
            Video::Format format;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            Video::ViewPort viewPort;

        public:
            Target(Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : format(format)
                , width(width)
                , height(height)
                , depth(depth)
                , viewPort(Math::Float2(0.0f, 0.0f), Math::Float2(float(width), float(height)), 0.0f, 1.0f)
            {
            }

            virtual ~Target(void) = default;

            // Video::Texture
            Video::Format getFormat(void)
            {
                return format;
            }

            uint32_t getWidth(void)
            {
                return width;
            }

            uint32_t getHeight(void)
            {
                return height;
            }

            uint32_t getDepth(void)
            {
                return depth;
            }

            // Video::Target
            const Video::ViewPort &getViewPort(void)
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
            TargetTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Target(format, width, height, depth)
                , Resource(d3dResource)
                , RenderTargetView(d3dRenderTargetView)
            {
            }

            virtual ~TargetTexture(void) = default;

            virtual void setName(const wchar_t *name)
            {
                setDebugName(d3dResource, name);
                setDebugName(d3dRenderTargetView, name);
            }
        };

        class TargetViewTexture
            : virtual public TargetTexture
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            TargetViewTexture(ID3D11Resource *d3dResource, ID3D11RenderTargetView *d3dRenderTargetView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : TargetTexture(d3dResource, d3dRenderTargetView, format, width, height, depth)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
            {
            }

            virtual ~TargetViewTexture(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(d3dResource, name);
                setDebugName(d3dRenderTargetView, name);
                setDebugName(d3dShaderResourceView, name);
                setDebugName(d3dUnorderedAccessView, name);
            }
        };

        class DepthTexture
            : public Texture
            , public Resource
            , public ShaderResourceView
            , public UnorderedAccessView
        {
        public:
            CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;

        public:
            DepthTexture(ID3D11Resource *d3dResource, ID3D11DepthStencilView *d3dDepthStencilView, ID3D11ShaderResourceView *d3dShaderResourceView, ID3D11UnorderedAccessView *d3dUnorderedAccessView, Video::Format format, uint32_t width, uint32_t height, uint32_t depth)
                : Texture(format, width, height, depth)
                , Resource(d3dResource)
                , ShaderResourceView(d3dShaderResourceView)
                , UnorderedAccessView(d3dUnorderedAccessView)
                , d3dDepthStencilView(d3dDepthStencilView)
            {
            }

            virtual ~DepthTexture(void) = default;

            void setName(const wchar_t *name)
            {
                setDebugName(d3dResource, name);
                setDebugName(d3dShaderResourceView, name);
                setDebugName(d3dUnorderedAccessView, name);
                setDebugName(d3dDepthStencilView, name);
            }
        };

        class Include
            : public ID3DInclude
        {
        public:
            const std::function<bool(const wchar_t *, String &)> &onInclude;
            std::unordered_map<StringUTF8, StringUTF8> includeMap;

        public:
            Include(const std::function<bool(const wchar_t *, String &)> &onInclude)
                : onInclude(onInclude)
            {
            }

            // ID3DInclude
            STDMETHODIMP Open(D3D_INCLUDE_TYPE includeType, const char *fileName, LPCVOID parentData, LPCVOID *data, UINT *dataSize)
            {
                String buffer;
                if (onInclude(String(fileName), buffer))
                {
                    auto &cache = includeMap[fileName] = buffer;
                    (*data) = cache.data();
                    (*dataSize) = cache.size();
                    return S_OK;
                }

                return E_FAIL;
            }

            STDMETHODIMP Close(LPCVOID data)
            {
                return S_OK;
            }
        };

        GEK_CONTEXT_USER(Device, HWND, bool, Video::Format, String)
            , public Video::Device
        {
            class Context
                : public Video::Device::Context
            {
                class ComputePipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext;

                public:
                    ComputePipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}
					
					void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->CSSetShader(program ? dynamic_cast<ComputeProgram *>(program)->d3dShader : nullptr, nullptr, 0);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11SamplerState *list[1] = { samplerState ? dynamic_cast<SamplerState *>(samplerState)->d3dSamplerState.p : nullptr };
                        d3dDeviceContext->CSSetSamplers(stage, 1, list);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11Buffer *list[1] = { buffer ? dynamic_cast<Buffer *>(buffer)->d3dBuffer.p : nullptr };
                        d3dDeviceContext->CSSetConstantBuffers(stage, 1, list);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    std::vector<ID3D11ShaderResourceView *> resourceCache;
                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            resourceCache[resource] = (resourceList && resourceList[resource] ? dynamic_cast<ShaderResourceView *>(resourceList[resource])->d3dShaderResourceView.p : nullptr);
                        }

                        d3dDeviceContext->CSSetShaderResources(firstStage, resourceCount, resourceCache.data());
                    }

                    std::vector<ID3D11UnorderedAccessView *> unorderedAccessCache;
                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.resize(std::max(unorderedAccessCount, unorderedAccessCache.size()));
                        for (uint32_t unorderedAccess = 0; unorderedAccess < unorderedAccessCount; unorderedAccess++)
                        {
                            unorderedAccessCache[unorderedAccess] = (unorderedAccessList && unorderedAccessList[unorderedAccess] ? dynamic_cast<UnorderedAccessView *>(unorderedAccessList[unorderedAccess])->d3dUnorderedAccessView.p : nullptr);
                        }

                        d3dDeviceContext->CSSetUnorderedAccessViews(firstStage, unorderedAccessCount, unorderedAccessCache.data(), countList);
                    }
                };

                class VertexPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext;

                public:
                    VertexPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
					void setInputLayout(Video::Object *inputLayout)
					{
						GEK_REQUIRE(d3dDeviceContext);

						d3dDeviceContext->IASetInputLayout(inputLayout ? dynamic_cast<InputLayout *>(inputLayout)->d3dInputLayout.p : nullptr);
					}

                    void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->VSSetShader(program ? dynamic_cast<VertexProgram *>(program)->d3dShader.p : nullptr, nullptr, 0);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11SamplerState *list[1] = { samplerState ? dynamic_cast<SamplerState *>(samplerState)->d3dSamplerState.p : nullptr };
                        d3dDeviceContext->VSSetSamplers(stage, 1, list);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11Buffer *list[1] = { buffer ? dynamic_cast<Buffer *>(buffer)->d3dBuffer.p : nullptr };
                        d3dDeviceContext->VSSetConstantBuffers(stage, 1, list);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    std::vector<ID3D11ShaderResourceView *> resourceCache;
                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            resourceCache[resource] = (resourceList && resourceList[resource] ? dynamic_cast<ShaderResourceView *>(resourceList[resource])->d3dShaderResourceView.p : nullptr);
                        }

                        d3dDeviceContext->VSSetShaderResources(firstStage, resourceCount, resourceCache.data());
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation();
                    }
                };

                class GeometryPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext;

                public:
                    GeometryPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}

					void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->GSSetShader(program ? dynamic_cast<GeometryProgram *>(program)->d3dShader.p : nullptr, nullptr, 0);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11SamplerState *list[1] = { samplerState ? dynamic_cast<SamplerState *>(samplerState)->d3dSamplerState.p : nullptr };
                        d3dDeviceContext->GSSetSamplers(stage, 1, list);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11Buffer *list[1] = { buffer ? dynamic_cast<Buffer *>(buffer)->d3dBuffer.p : nullptr };
                        d3dDeviceContext->GSSetConstantBuffers(stage, 1, list);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    std::vector<ID3D11ShaderResourceView *> resourceCache;
                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            resourceCache[resource] = (resourceList && resourceList[resource] ? dynamic_cast<ShaderResourceView *>(resourceList[resource])->d3dShaderResourceView.p : nullptr);
                        }

                        d3dDeviceContext->GSSetShaderResources(firstStage, resourceCount, resourceCache.data());
                    }

                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        throw Video::UnsupportedOperation();
                    }
                };

                class PixelPipeline
                    : public Video::Device::Context::Pipeline
                {
                private:
                    ID3D11DeviceContext *d3dDeviceContext;

                public:
                    PixelPipeline(ID3D11DeviceContext *d3dDeviceContext)
                        : d3dDeviceContext(d3dDeviceContext)
                    {
                        GEK_REQUIRE(d3dDeviceContext);
                    }

                    // Video::Pipeline
					void setInputLayout(Video::Object *inputLayout)
					{
						throw Video::UnsupportedOperation();
					}

					void setProgram(Video::Object *program)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        d3dDeviceContext->PSSetShader(program ? dynamic_cast<PixelProgram *>(program)->d3dShader.p : nullptr, nullptr, 0);
                    }

                    void setSamplerState(Video::Object *samplerState, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11SamplerState *list[1] = { samplerState ? dynamic_cast<SamplerState *>(samplerState)->d3dSamplerState.p : nullptr };
                        d3dDeviceContext->PSSetSamplers(stage, 1, list);
                    }

                    void setConstantBuffer(Video::Buffer *buffer, uint32_t stage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        ID3D11Buffer *list[1] = { buffer ? dynamic_cast<Buffer *>(buffer)->d3dBuffer.p : nullptr };
                        d3dDeviceContext->PSSetConstantBuffers(stage, 1, list);
                    }

                    void setResource(Video::Object *resource, uint32_t firstStage)
                    {
                        setResourceList(&resource, 1, firstStage);
                    }

                    void setUnorderedAccess(Video::Object *unorderedAccess, uint32_t firstStage, uint32_t count)
                    {
                        setUnorderedAccessList(&unorderedAccess, 1, firstStage, &count);
                    }

                    std::vector<ID3D11ShaderResourceView *> resourceCache;
                    void setResourceList(Video::Object **resourceList, uint32_t resourceCount, uint32_t firstStage)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        resourceCache.resize(std::max(resourceCount, resourceCache.size()));
                        for (uint32_t resource = 0; resource < resourceCount; resource++)
                        {
                            resourceCache[resource] = (resourceList && resourceList[resource] ? dynamic_cast<ShaderResourceView *>(resourceList[resource])->d3dShaderResourceView.p : nullptr);
                        }

                        d3dDeviceContext->PSSetShaderResources(firstStage, resourceCount, resourceCache.data());
                    }

                    std::vector<ID3D11UnorderedAccessView *> unorderedAccessCache;
                    void setUnorderedAccessList(Video::Object **unorderedAccessList, uint32_t unorderedAccessCount, uint32_t firstStage, uint32_t *countList)
                    {
                        GEK_REQUIRE(d3dDeviceContext);

                        unorderedAccessCache.resize(std::max(unorderedAccessCount, unorderedAccessCache.size()));
                        for (uint32_t unorderedAccess = 0; unorderedAccess < unorderedAccessCount; unorderedAccess++)
                        {
                            unorderedAccessCache[unorderedAccess] = (unorderedAccessList && unorderedAccessList[unorderedAccess] ? dynamic_cast<UnorderedAccessView *>(unorderedAccessList[unorderedAccess])->d3dUnorderedAccessView.p : nullptr);
                        }

                        d3dDeviceContext->OMSetRenderTargetsAndUnorderedAccessViews(D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL, nullptr, nullptr, firstStage, unorderedAccessCount, unorderedAccessCache.data(), countList);
                    }
                };

            public:
                Device *device;
                CComPtr<ID3D11DeviceContext> d3dDeviceContext;
                PipelinePtr computeSystemHandler;
                PipelinePtr vertexSystemHandler;
                PipelinePtr geomtrySystemHandler;
                PipelinePtr pixelSystemHandler;

            public:
                Context(Device *device, ID3D11DeviceContext *d3dDeviceContext)
                    : device(device)
                    , d3dDeviceContext(d3dDeviceContext)
                    , computeSystemHandler(new ComputePipeline(d3dDeviceContext))
                    , vertexSystemHandler(new VertexPipeline(d3dDeviceContext))
                    , geomtrySystemHandler(new GeometryPipeline(d3dDeviceContext))
                    , pixelSystemHandler(new PixelPipeline(d3dDeviceContext))
                {
                    GEK_REQUIRE(device);
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(computeSystemHandler);
                    GEK_REQUIRE(vertexSystemHandler);
                    GEK_REQUIRE(geomtrySystemHandler);
                    GEK_REQUIRE(pixelSystemHandler);
                }

                // Video::Context
                Device * const getDevice(void)
                {
                    return device;
                }

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

                    d3dDeviceContext->GenerateMips(dynamic_cast<ShaderResourceView *>(texture)->d3dShaderResourceView);
                }

                void clearState(void)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dDeviceContext->ClearState();
                }

                void setViewports(const Video::ViewPort *viewPortList, uint32_t viewPortCount)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    if (viewPortCount)
                    {
                        d3dDeviceContext->RSSetViewports(viewPortCount, (D3D11_VIEWPORT *)viewPortList);
                    }
                }

                void setScissorRect(const Shapes::Rectangle<uint32_t> *rectangleList, uint32_t rectangleCount)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    if (rectangleCount > 0)
                    {
                        d3dDeviceContext->RSSetScissorRects(rectangleCount, (D3D11_RECT *)rectangleList);
                    }
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

                    auto unorderedAccessView = dynamic_cast<UnorderedAccessView *>(object);
                    d3dDeviceContext->ClearUnorderedAccessViewFloat(unorderedAccessView->d3dUnorderedAccessView.p, value.data);
                }

                void clearUnorderedAccess(Video::Object *object, const uint32_t value[4])
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(object);

                    auto unorderedAccessView = dynamic_cast<UnorderedAccessView *>(object);
                    d3dDeviceContext->ClearUnorderedAccessViewUint(unorderedAccessView->d3dUnorderedAccessView.p, value);
                }

                void clearRenderTarget(Video::Target *renderTarget, const Math::Color &clearColor)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(renderTarget);

                    auto targetViewTexture = dynamic_cast<TargetViewTexture *>(renderTarget);
                    d3dDeviceContext->ClearRenderTargetView(targetViewTexture->d3dRenderTargetView.p, clearColor.data);
                }

                void clearDepthStencilTarget(Video::Object *depthBuffer, uint32_t flags, float clearDepth, uint32_t clearStencil)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(depthBuffer);

                    auto depthTexture = dynamic_cast<DepthTexture *>(depthBuffer);
                    d3dDeviceContext->ClearDepthStencilView(depthTexture->d3dDepthStencilView,
                        ((flags & Video::ClearFlags::Depth ? D3D11_CLEAR_DEPTH : 0) |
                        (flags & Video::ClearFlags::Stencil ? D3D11_CLEAR_STENCIL : 0)),
                        clearDepth, clearStencil);
                }

                std::vector<ID3D11RenderTargetView *> d3dRenderTargetViewCache;
                void setRenderTargets(Video::Target **renderTargetList, uint32_t renderTargetCount, Video::Object *depthBuffer)
                {
                    GEK_REQUIRE(d3dDeviceContext);

                    d3dRenderTargetViewCache.resize(std::max(renderTargetCount, d3dRenderTargetViewCache.size()));
                    for (uint32_t renderTarget = 0; renderTarget < renderTargetCount; renderTarget++)
                    {
                        d3dRenderTargetViewCache[renderTarget] = (renderTargetList && renderTargetList[renderTarget] ? dynamic_cast<RenderTargetView *>(renderTargetList[renderTarget])->d3dRenderTargetView.p : nullptr);
                    }

                    d3dDeviceContext->OMSetRenderTargets(renderTargetCount, d3dRenderTargetViewCache.data(), (depthBuffer ? dynamic_cast<DepthTexture *>(depthBuffer)->d3dDepthStencilView.p : nullptr));
                }

                void setRenderState(Video::Object *renderState)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(renderState);

                    d3dDeviceContext->RSSetState(dynamic_cast<RenderState *>(renderState)->d3dRenderState);
                }

                void setDepthState(Video::Object *depthState, uint32_t stencilReference)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(depthState);

                    d3dDeviceContext->OMSetDepthStencilState(dynamic_cast<DepthState *>(depthState)->d3dDepthState, stencilReference);
                }

                void setBlendState(Video::Object *blendState, const Math::Color &blendFactor, uint32_t mask)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(blendState);

                    d3dDeviceContext->OMSetBlendState(dynamic_cast<BlendState *>(blendState)->d3dBlendState, blendFactor.data, mask);
                }

                void setVertexBuffer(uint32_t slot, Video::Buffer *vertexBuffer, uint32_t offset)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(vertexBuffer);

                    uint32_t stride = vertexBuffer->getStride();
                    d3dDeviceContext->IASetVertexBuffers(slot, 1, &dynamic_cast<Buffer *>(vertexBuffer)->d3dBuffer.p, &stride, &offset);
                }

                void setIndexBuffer(Video::Buffer *indexBuffer, uint32_t offset)
                {
                    GEK_REQUIRE(d3dDeviceContext);
                    GEK_REQUIRE(indexBuffer);

                    DXGI_FORMAT format = DirectX::BufferFormatList[static_cast<uint8_t>(indexBuffer->getFormat())];
                    d3dDeviceContext->IASetIndexBuffer(dynamic_cast<Buffer *>(indexBuffer)->d3dBuffer, format, offset);
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
                        throw Video::OperationFailed();
                    }

                    return std::make_shared<CommandList>(d3dCommandList.p);
                }
            };

        public:
            HWND window;
            bool isChildWindow;
            bool fullScreen;
            Video::Format backBufferFormat;

            CComPtr<ID3D11Device> d3dDevice;
            CComPtr<ID3D11DeviceContext> d3dDeviceContext;
            CComPtr<IDXGISwapChain> dxSwapChain;

            Video::Device::ContextPtr defaultContext;
            Video::TargetPtr backBuffer;

        public:
			Device(Gek::Context *context, HWND window, bool fullScreen, Video::Format backBufferFormat, String device)
				: ContextRegistration(context)
				, window(window)
				, isChildWindow(GetParent(window) != nullptr)
				, fullScreen(fullScreen)
				, backBufferFormat(backBufferFormat)
            {
                GEK_REQUIRE(window);

                DXGI_SWAP_CHAIN_DESC swapChainDescription;
                swapChainDescription.BufferDesc.Width = 0;
                swapChainDescription.BufferDesc.Height = 0;
                swapChainDescription.BufferDesc.Format = DirectX::TextureFormatList[static_cast<uint8_t>(backBufferFormat)];
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
                if (featureLevel != featureLevelList[0])
                {
                    throw Video::FeatureLevelNotSupported();
                }

                if (FAILED(resultValue) || !dxSwapChain || !d3dDevice || !d3dDeviceContext)
                {
                    throw Video::InitializationFailed();
                }

                CComQIPtr<IDXGIDevice> dxgiDevice(d3dDevice);
                if (!dxgiDevice)
                {
                    throw Video::InitializationFailed();
                }

                CComPtr<IDXGIAdapter> dxgiAdapter;
                dxgiDevice->GetParent(IID_PPV_ARGS(&dxgiAdapter));
                if (!dxgiAdapter)
                {
                    throw Video::InitializationFailed();
                }

                CComPtr<IDXGIFactory> dxgiFactory;
                dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
                if (!dxgiFactory)
                {
                    throw Video::InitializationFailed();
                }

                dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);

#ifdef _DEBUG
                CComQIPtr<ID3D11Debug> d3dDebug(d3dDevice);
                CComQIPtr<ID3D11InfoQueue> d3dInfoQueue(d3dDebug);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
#endif

                if (fullScreen && !isChildWindow)
                {
                    resultValue = dxSwapChain->SetFullscreenState(true, nullptr);
                    if (FAILED(resultValue))
                    {
                        throw Video::OperationFailed();
                    }
                }

                defaultContext = std::make_shared<Context>(this, d3dDeviceContext);
            }

            ~Device(void)
            {
                setFullScreen(false);

                backBuffer = nullptr;
                defaultContext = nullptr;

                dxSwapChain.Release();
                d3dDeviceContext.Release();
                d3dDevice.Release();
            }

            // System
            void setFullScreen(bool fullScreen)
            {
                if (!isChildWindow && this->fullScreen != fullScreen)
                {
                    this->fullScreen = fullScreen;
                    HRESULT resultValue = dxSwapChain->SetFullscreenState(fullScreen, nullptr);
                    if (FAILED(resultValue))
                    {
                        throw Video::OperationFailed();
                    }
                }
            }

            void setSize(uint32_t width, uint32_t height, Video::Format backBufferFormat)
            {
                GEK_REQUIRE(dxSwapChain);

                DXGI_SWAP_CHAIN_DESC chainDescription;
                DXGI_MODE_DESC &modeDescription = chainDescription.BufferDesc;
                dxSwapChain->GetDesc(&chainDescription);
                if (width != modeDescription.Width ||
                    height != modeDescription.Height ||
                    DirectX::TextureFormatList[static_cast<uint8_t>(backBufferFormat)] != modeDescription.Format)
                {
                    this->backBufferFormat = backBufferFormat;

                    DXGI_MODE_DESC description;
                    description.Width = width;
                    description.Height = height;
                    description.Format = DirectX::TextureFormatList[static_cast<uint8_t>(backBufferFormat)];
                    description.RefreshRate.Numerator = 60;
                    description.RefreshRate.Denominator = 1;
                    description.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
                    description.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
                    HRESULT resultValue = dxSwapChain->ResizeTarget(&description);
                    if (FAILED(resultValue))
                    {
                        throw Video::OperationFailed();
                    }
                }
            }

            void resize(void)
            {
                GEK_REQUIRE(dxSwapChain);

                backBuffer = nullptr;
                HRESULT resultValue = dxSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
                if (FAILED(resultValue))
                {
                    throw Video::OperationFailed();
                }
            }

			const char * const getSemanticMoniker(Video::InputElement::Semantic semantic)
			{
				switch (semantic)
				{
				case Video::InputElement::Semantic::Position: return "POSITION";
				case Video::InputElement::Semantic::TexCoord: return "TEXCOORD";
				case Video::InputElement::Semantic::Tangent: return "TANGENT";
				case Video::InputElement::Semantic::BiTangnet: return "BINORMAL";
				case Video::InputElement::Semantic::Normal: return "NORMAL";
				case Video::InputElement::Semantic::Color: return "COLOR";
				default: return "TEXCOORD";
				};
			}

            void * const getSwapChain(void)
            {
                return static_cast<void *>(dxSwapChain.p);
            }

            Video::Target * const getBackBuffer(void)
            {
                if (!backBuffer)
                {
                    CComPtr<ID3D11Texture2D> d3dRenderTarget;
                    HRESULT resultValue = dxSwapChain->GetBuffer(0, IID_PPV_ARGS(&d3dRenderTarget));
                    if (FAILED(resultValue) || !d3dRenderTarget)
                    {
                        throw Video::OperationFailed();
                    }

                    CComPtr<ID3D11RenderTargetView> d3dRenderTargetView;
                    resultValue = d3dDevice->CreateRenderTargetView(d3dRenderTarget, nullptr, &d3dRenderTargetView);
                    if (FAILED(resultValue) || !d3dRenderTargetView)
                    {
                        throw Video::OperationFailed();
                    }

                    D3D11_TEXTURE2D_DESC description;
                    d3dRenderTarget->GetDesc(&description);
                    backBuffer = std::make_shared<TargetTexture>(d3dRenderTarget.p, d3dRenderTargetView.p, backBufferFormat, description.Width, description.Height, 1);
                }

                return backBuffer.get();
            }

            Video::Device::Context * const getDefaultContext(void)
            {
                GEK_REQUIRE(defaultContext);

                return defaultContext.get();
            }

            bool isFullScreen(void)
            {
                return fullScreen;
            }

            Video::Device::ContextPtr createDeferredContext(void)
            {
                GEK_REQUIRE(d3dDevice);

                CComPtr<ID3D11DeviceContext> d3dDeferredDeviceContext;
                HRESULT resultValue = d3dDevice->CreateDeferredContext(0, &d3dDeferredDeviceContext);
                if (FAILED(resultValue) || !d3dDeferredDeviceContext)
                {
                    throw Video::OperationFailed();
                }

                return std::make_shared<Context>(this, d3dDeferredDeviceContext.p);
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
                    throw Video::OperationFailed();
                }

                return std::make_shared<Event>(d3dQuery);
            }

            void setEvent(Video::Object *event)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(event);

                d3dDeviceContext->End(dynamic_cast<Event *>(event)->d3dQuery);
            }

            bool isEventSet(Video::Object *event)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(event);

                uint32_t isEventSet = 0;
                if (FAILED(d3dDeviceContext->GetData(dynamic_cast<Event *>(event)->d3dQuery, (LPVOID)&isEventSet, sizeof(uint32_t), TRUE)))
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
                    throw Video::CreateObjectFailed();
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
                    throw Video::CreateObjectFailed();
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
                    throw Video::CreateObjectFailed();
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
                    throw Video::CreateObjectFailed();
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
                    throw Video::CreateObjectFailed();
                }

                return std::make_shared<SamplerState>(d3dStates);
            }

            Video::BufferPtr createBuffer(Video::Format format, uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
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
                    if (FAILED(resultValue) || !d3dBuffer)
                    {
                        throw Video::CreateObjectFailed();
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
                        throw Video::CreateObjectFailed();
                    }
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (flags & Video::BufferFlags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(format)];
                    viewDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = count;
                    HRESULT resultValue = d3dDevice->CreateShaderResourceView(d3dBuffer, &viewDescription, &d3dShaderResourceView);
                    if (FAILED(resultValue) || !d3dShaderResourceView)
                    {
                        throw Video::CreateObjectFailed();
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (flags & Video::BufferFlags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::BufferFormatList[static_cast<uint8_t>(format)];
                    viewDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                    viewDescription.Buffer.FirstElement = 0;
                    viewDescription.Buffer.NumElements = count;
                    viewDescription.Buffer.Flags = (flags & Video::BufferFlags::Counter ? D3D11_BUFFER_UAV_FLAG_COUNTER : 0);

                    HRESULT resultValue = d3dDevice->CreateUnorderedAccessView(d3dBuffer, &viewDescription, &d3dUnorderedAccessView);
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        throw Video::CreateObjectFailed();
                    }
                }

                return std::make_shared<Buffer>(d3dBuffer, d3dShaderResourceView, d3dUnorderedAccessView, format, stride, count);
            }

            Video::BufferPtr createBuffer(uint32_t stride, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
            {
                return createBuffer(Video::Format::Unknown, stride, count, type, flags, data);
            }

            Video::BufferPtr createBuffer(Video::Format format, uint32_t count, Video::BufferType type, uint32_t flags, const void *data)
            {
                uint32_t stride = DirectX::FormatStrideList[static_cast<uint8_t>(format)];
                return createBuffer(format, stride, count, type, flags, data);
            }

            void mapBuffer(Video::Buffer *buffer, void **data, Video::Map mapping)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(data);

                D3D11_MAP d3dMapping = DirectX::MapList[static_cast<uint8_t>(mapping)];

                D3D11_MAPPED_SUBRESOURCE mappedSubResource;
                mappedSubResource.pData = nullptr;
                mappedSubResource.RowPitch = 0;
                mappedSubResource.DepthPitch = 0;

                HRESULT resultValue = d3dDeviceContext->Map(dynamic_cast<Buffer *>(buffer)->d3dBuffer, 0, d3dMapping, 0, &mappedSubResource);
                if (FAILED(resultValue))
                {
                    throw Video::OperationFailed();
                }

                (*data) = mappedSubResource.pData;
            }

            void unmapBuffer(Video::Buffer *buffer)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(buffer);

                d3dDeviceContext->Unmap(dynamic_cast<Buffer *>(buffer)->d3dBuffer, 0);
            }

            void updateResource(Video::Object *object, const void *data)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(object);
                GEK_REQUIRE(data);

                d3dDeviceContext->UpdateSubresource(dynamic_cast<Resource *>(object)->d3dResource, 0, nullptr, data, 0, 0);
            }

            void copyResource(Video::Object *destination, Video::Object *source)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(destination);
                GEK_REQUIRE(source);

                d3dDeviceContext->CopyResource(dynamic_cast<Resource *>(destination)->d3dResource, dynamic_cast<Resource *>(source)->d3dResource);
            }

			Video::ObjectPtr createInputLayout(const std::vector<Video::InputElement> &elementList, const void *compiledData, uint32_t compiledSize)
			{
				static const char *SemanticNameList[] =
				{
					"Position",
					"TexCoord",
					"Tangent",
					"BiTangnet",
					"Normal",
					"Color",
				};

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
					elementDesc.SemanticName = SemanticNameList[static_cast<uint8_t>(element.semantic)];
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
				HRESULT resultValue = d3dDevice->CreateInputLayout(d3dElementList.data(), d3dElementList.size(), compiledData, compiledSize, &d3dInputLayout);
				if (FAILED(resultValue) || !d3dInputLayout)
				{
					throw Video::CreateObjectFailed();
				}

				return std::make_shared<InputLayout>(d3dInputLayout);
			}

			template <class SHADER, class PROGRAM, typename RETURN, typename CLASS, typename... ARGUMENTS>
			Video::ObjectPtr createProgram(const void *compiledData, uint32_t compiledSize, RETURN(__stdcall CLASS::*function)(ARGUMENTS...))
			{
				CComPtr<SHADER> d3dShader;
				HRESULT resultValue = (d3dDevice->*function)(compiledData, compiledSize, nullptr, &d3dShader);
				if (FAILED(resultValue) || !d3dShader)
				{
					throw Video::CreateObjectFailed();
				}

				return std::make_shared<PROGRAM>(d3dShader);
			}

			Video::ObjectPtr createComputeProgram(const void *compiledData, uint32_t compiledSize)
			{
				return createProgram<ID3D11ComputeShader, ComputeProgram>(compiledData, compiledSize, &ID3D11Device::CreateComputeShader);
			}

			Video::ObjectPtr createVertexProgram(const void *compiledData, uint32_t compiledSize)
			{
				return createProgram<ID3D11VertexShader, VertexProgram>(compiledData, compiledSize, &ID3D11Device::CreateVertexShader);
			}

			Video::ObjectPtr createGeometryProgram(const void *compiledData, uint32_t compiledSize)
			{
				return createProgram<ID3D11GeometryShader, GeometryProgram>(compiledData, compiledSize, &ID3D11Device::CreateGeometryShader);
			}

			Video::ObjectPtr createPixelProgram(const void *compiledData, uint32_t compiledSize)
			{
				return createProgram<ID3D11PixelShader, PixelProgram>(compiledData, compiledSize, &ID3D11Device::CreatePixelShader);
			}

			std::vector<uint8_t> compileProgram(const StringUTF8 &name, const StringUTF8 &type, const StringUTF8 &uncompiledProgram, const StringUTF8 &entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
			{
				GEK_REQUIRE(d3dDevice);

				uint32_t flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#ifdef _DEBUG
				flags |= D3DCOMPILE_DEBUG;
#endif

				Include include(onInclude);
				CComPtr<ID3DBlob> d3dShaderBlob;
				CComPtr<ID3DBlob> d3dCompilerErrors;
				HRESULT resultValue = D3DCompile(uncompiledProgram, (uncompiledProgram.size() + 1), name, nullptr, &include, entryFunction, type, flags, 0, &d3dShaderBlob, &d3dCompilerErrors);
				if (FAILED(resultValue) || !d3dShaderBlob)
				{
					String errors = (const char *)d3dCompilerErrors->GetBufferPointer();
					throw Video::ProgramCompilationFailed();
				}

				uint8_t *data = (uint8_t *)d3dShaderBlob->GetBufferPointer();
				return std::vector<uint8_t>(data, (data + d3dShaderBlob->GetBufferSize()));
			}

			std::vector<uint8_t> compileComputeProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
				return compileProgram(name, "cs_5_0", uncompiledProgram, entryFunction, onInclude);
            }

			std::vector<uint8_t> compileVertexProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
				return compileProgram(name, "vs_5_0", uncompiledProgram, entryFunction, onInclude);
			}

			std::vector<uint8_t> compileGeometryProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
				return compileProgram(name, "gs_5_0", uncompiledProgram, entryFunction, onInclude);
			}

			std::vector<uint8_t> compilePixelProgram(const wchar_t *name, const wchar_t *uncompiledProgram, const wchar_t *entryFunction, const std::function<bool(const wchar_t *, String &)> &onInclude)
            {
				return compileProgram(name, "ps_5_0", uncompiledProgram, entryFunction, onInclude);
			}

            Video::TexturePtr createTexture(Video::Format format, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipmaps, uint32_t flags, const void *data)
            {
                GEK_REQUIRE(d3dDevice);
                GEK_REQUIRE(format != Video::Format::Unknown);
                GEK_REQUIRE(width != 0);
                GEK_REQUIRE(height != 0);
                GEK_REQUIRE(depth != 0);

                uint32_t bindFlags = 0;
                if (flags & Video::TextureFlags::RenderTarget)
                {
                    if (flags & Video::TextureFlags::DepthTarget)
                    {
                        throw Video::InvalidParameters();
                    }

                    bindFlags |= D3D11_BIND_RENDER_TARGET;
                }

                if (flags & Video::TextureFlags::DepthTarget)
                {
                    if (flags & Video::TextureFlags::RenderTarget || depth > 1)
                    {
                        throw Video::InvalidParameters();
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

                D3D11_SUBRESOURCE_DATA resourceData;
                resourceData.pSysMem = data;
                resourceData.SysMemPitch = (DirectX::FormatStrideList[static_cast<uint8_t>(format)] * width);
                resourceData.SysMemSlicePitch = (depth == 1 ? 0 : (resourceData.SysMemPitch * height));

                CComQIPtr<ID3D11Resource> d3dResource;
                if (depth == 1)
                {
                    D3D11_TEXTURE2D_DESC textureDescription;
                    textureDescription.Width = width;
                    textureDescription.Height = height;
                    textureDescription.MipLevels = mipmaps;
                    textureDescription.Format = DirectX::TextureFormatList[static_cast<uint8_t>(format)];
                    textureDescription.ArraySize = 1;
                    textureDescription.SampleDesc.Count = 1;
                    textureDescription.SampleDesc.Quality = 0;
                    textureDescription.BindFlags = bindFlags;
                    textureDescription.CPUAccessFlags = 0;
                    textureDescription.MiscFlags = (mipmaps == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);
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
                        throw Video::CreateObjectFailed();
                    }

                    d3dResource = texture2D;
                }
                else
                {
                    D3D11_TEXTURE3D_DESC textureDescription;
                    textureDescription.Width = width;
                    textureDescription.Height = height;
                    textureDescription.Depth = depth;
                    textureDescription.MipLevels = mipmaps;
                    textureDescription.Format = DirectX::TextureFormatList[static_cast<uint8_t>(format)];
                    textureDescription.BindFlags = bindFlags;
                    textureDescription.CPUAccessFlags = 0;
                    textureDescription.MiscFlags = (mipmaps == 1 ? 0 : D3D11_RESOURCE_MISC_GENERATE_MIPS);
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
                        throw Video::CreateObjectFailed();
                    }

                    d3dResource = texture3D;
                }

                if (!d3dResource)
                {
                    throw Video::CreateObjectFailed();
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                if (flags & Video::TextureFlags::Resource)
                {
                    D3D11_SHADER_RESOURCE_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(format)];
                    if (depth == 1)
                    {
                        viewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
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
                        throw Video::CreateObjectFailed();
                    }
                }

                CComPtr<ID3D11UnorderedAccessView> d3dUnorderedAccessView;
                if (flags & Video::TextureFlags::UnorderedAccess)
                {
                    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDescription;
                    viewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(format)];
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
                    if (FAILED(resultValue) || !d3dUnorderedAccessView)
                    {
                        throw Video::CreateObjectFailed();
                    }
                }

                if (flags & Video::TextureFlags::RenderTarget)
                {
                    D3D11_RENDER_TARGET_VIEW_DESC renderViewDescription;
                    renderViewDescription.Format = DirectX::ViewFormatList[static_cast<uint8_t>(format)];
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
                    if (FAILED(resultValue) || !d3dRenderTargetView)
                    {
                        throw Video::CreateObjectFailed();
                    }

                    return std::make_shared<TargetViewTexture>(d3dResource.p, d3dRenderTargetView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth);
                }
                else if (flags & Video::TextureFlags::DepthTarget)
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilDescription;
                    depthStencilDescription.Format = DirectX::DepthFormatList[static_cast<uint8_t>(format)];
                    depthStencilDescription.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilDescription.Flags = 0;
                    depthStencilDescription.Texture2D.MipSlice = 0;

                    CComPtr<ID3D11DepthStencilView> d3dDepthStencilView;
                    HRESULT resultValue = d3dDevice->CreateDepthStencilView(d3dResource, &depthStencilDescription, &d3dDepthStencilView);
                    if (FAILED(resultValue) || !d3dDepthStencilView)
                    {
                        throw Video::CreateObjectFailed();
                    }

                    return std::make_shared<DepthTexture>(d3dResource.p, d3dDepthStencilView.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth);
                }
                else
                {
                    return std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, d3dUnorderedAccessView.p, format, width, height, depth);
                }
            }

            Video::TexturePtr loadTexture(const wchar_t *fileName, uint32_t flags)
            {
                GEK_REQUIRE(d3dDevice);

                std::vector<uint8_t> buffer;
                FileSystem::load(fileName, buffer);

				String extension(FileSystem::getExtension(fileName));
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
                    throw Video::InvalidFileType();
                }

				::DirectX::ScratchImage image;
				HRESULT resultValue = load(buffer, image);
                if (FAILED(resultValue))
                {
                    throw Video::LoadFileFailed();
                }

                CComPtr<ID3D11ShaderResourceView> d3dShaderResourceView;
                resultValue = ::DirectX::CreateShaderResourceViewEx(d3dDevice, image.GetImages(), image.GetImageCount(), image.GetMetadata(), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, (flags & Video::TextureLoadFlags::sRGB), &d3dShaderResourceView);
                if (FAILED(resultValue) || !d3dShaderResourceView)
                {
                    throw Video::CreateObjectFailed();
                }

                CComPtr<ID3D11Resource> d3dResource;
                d3dShaderResourceView->GetResource(&d3dResource);
                if (FAILED(resultValue) || !d3dResource)
                {
                    throw Video::CreateObjectFailed();
                }

                return std::make_shared<ViewTexture>(d3dResource.p, d3dShaderResourceView.p, nullptr, Video::Format::Unknown, image.GetMetadata().width, image.GetMetadata().height, image.GetMetadata().depth);
            }

            void executeCommandList(Video::Object *commandList)
            {
                GEK_REQUIRE(d3dDeviceContext);
                GEK_REQUIRE(commandList);

                CComQIPtr<ID3D11CommandList> d3dCommandList;
                d3dDeviceContext->ExecuteCommandList(dynamic_cast<CommandList *>(commandList)->d3dCommandList, FALSE);
            }

            void present(bool waitForVerticalSync)
            {
                GEK_REQUIRE(dxSwapChain);

                dxSwapChain->Present(waitForVerticalSync ? 1 : 0, 0);
            }
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // Direct3D11
}; // namespace Gek
