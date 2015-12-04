#pragma once

#include "GEK\System\VideoSystem.h"

namespace Gek
{
    DECLARE_INTERFACE_IID(Resources, "2B0EB375-460C-46E8-9B99-DB21AB54FBA5") : virtual public IUnknown
    {
        STDMETHOD(initialize)                           (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD_(std::size_t, loadPlugin)             (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(std::size_t, loadMaterial)           (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD(loadShader)                           (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;

        STDMETHOD(createRenderStates)                   (THIS_ IUnknown **returnObject, const Video::RenderStates &renderStates) PURE;
        STDMETHOD(createDepthStates)                    (THIS_ IUnknown **returnObject, const Video::DepthStates &depthStates) PURE;
        STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video::UnifiedBlendStates &blendStates) PURE;
        STDMETHOD(createBlendStates)                    (THIS_ IUnknown **returnObject, const Video::IndependentBlendStates &blendStates) PURE;
        STDMETHOD(createRenderTarget)                   (THIS_ VideoTexture **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD(createDepthTarget)                    (THIS_ IUnknown **returnObject, UINT32 width, UINT32 height, Video::Format format, UINT32 flags) PURE;
        STDMETHOD(createBuffer)                         (THIS_ VideoBuffer **returnObject, LPCWSTR name, UINT32 stride, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD(createBuffer)                         (THIS_ VideoBuffer **returnObject, LPCWSTR name, Video::Format format, UINT32 count, DWORD flags, LPCVOID staticData = nullptr) PURE;
        STDMETHOD(loadTexture)                          (THIS_ VideoTexture **returnObject, LPCWSTR fileName, UINT32 flags) PURE;
        STDMETHOD(loadComputeProgram)                   (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
        STDMETHOD(loadPixelProgram)                     (THIS_ IUnknown **returnObject, LPCWSTR fileName, LPCSTR entryFunction, std::function<HRESULT(LPCSTR, std::vector<UINT8> &)> onInclude = nullptr, std::unordered_map<CStringA, CStringA> *defineList = nullptr) PURE;
    };
}; // namespace Gek
