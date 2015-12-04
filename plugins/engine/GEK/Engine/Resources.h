#pragma once

#include "GEK\System\VideoSystem.h"

namespace Gek
{
    template <typename TYPE>
    struct Handle
    {
        TYPE identifier;

        Handle(UINT64 identifier = 0)
            : identifier(static_cast<TYPE>(identifier))
        {
        }

        operator bool() const
        {
            return (identifier == 0 ? false : true);
        }
    };

    typedef Handle<UINT16> ProgramHandle;
    typedef Handle<UINT8> PluginHandle;
    typedef Handle<UINT16> MaterialHandle;
    typedef Handle<UINT8> ShaderHandle;
    typedef Handle<UINT32> TextureHandle;
    typedef Handle<UINT16> BufferHandle;
    typedef Handle<UINT8> RenderStatesHandle;
    typedef Handle<UINT8> DepthStatesHandle;
    typedef Handle<UINT8> BlendStatesHandle;

    DECLARE_INTERFACE_IID(Resources, "2B0EB375-460C-46E8-9B99-DB21AB54FBA5") : virtual public IUnknown
    {
        STDMETHOD(initialize)                               (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD_(PluginHandle, loadPlugin)                (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(MaterialHandle, loadMaterial)            (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(ShaderHandle, loadShader)                (THIS_ LPCWSTR fileName) PURE;

        STDMETHOD_(RenderStatesHandle, createRenderStates)  (THIS_ const Video::RenderStates &renderStates) PURE;
        STDMETHOD_(DepthStatesHandle, createDepthStates)    (THIS_ const Video::DepthStates &depthStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD_(BlendStatesHandle, createBlendStates)    (THIS_ const Video::IndependentBlendStates &blendStates) PURE;
        STDMETHOD_(TextureHandle, createRenderTarget)       (THIS_ UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD_(TextureHandle, createDepthTarget)        (THIS_ UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD_(BufferHandle, createBuffer)              (THIS_ LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD_(BufferHandle, createBuffer)              (THIS_ LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD_(TextureHandle, loadTexture)              (THIS_ LPCWSTR fileName, UINT32 flags) PURE;
        STDMETHOD_(ProgramHandle, loadComputeProgram)       (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD_(ProgramHandle, loadPixelProgram)         (THIS_ LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;

        STDMETHOD_(void, setRenderStates)                   (THIS_ VideoContext *videoContext, RenderStatesHandle renderStatesHandle) PURE;
        STDMETHOD_(void, setDepthStates)                    (THIS_ VideoContext *videoContext, DepthStatesHandle depthStatesHandle, UINT32 stencilReference) PURE;
        STDMETHOD_(void, setBlendStates)                    (THIS_ VideoContext *videoContext, BlendStatesHandle blendStatesHandle, const Math::Float4 &blendFactor, UINT32 sampleMask) PURE;
        STDMETHOD_(void, setResource)                       (THIS_ VideoPipeline *videoPipeline, TextureHandle textureHandle, UINT32 stage) PURE;
        STDMETHOD_(void, setResource)                       (THIS_ VideoPipeline *videoPipeline, BufferHandle bufferHandle, UINT32 stage) PURE;
        STDMETHOD_(void, setProgram)                        (THIS_ VideoPipeline *videoPipeline, ProgramHandle programHandle) PURE;
        STDMETHOD_(void, setVertexBuffer)                   (THIS_ VideoContext *videoContext, UINT32 slot, BufferHandle bufferHandle, UINT32 offset) PURE;
        STDMETHOD_(void, setIndexBuffer)                    (THIS_ VideoContext *videoContext, BufferHandle bufferHandle, UINT32 offset) PURE;
        STDMETHOD_(void, clearRenderTarget)                 (THIS_ VideoContext *videoContext, TextureHandle textureHandle, const Math::Float4 &color) PURE;
    };
}; // namespace Gek


namespace std
{
    template <typename TYPE>
    struct hash<Gek::Handle<TYPE>>
    {
        size_t operator()(const Gek::Handle<TYPE> &value) const
        {
            return value.identifier;
        }
    };
};
