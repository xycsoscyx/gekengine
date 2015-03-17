#pragma once

#include "GEKContext.h"

typedef UINT32 GEKRESOURCEID;
const GEKRESOURCEID GEKINVALIDRESOURCEID = 0;

DECLARE_INTERFACE_IID_(IGEKResourceSystem, IUnknown, "4680DA6E-2FD2-4918-A635-497C09A3969E")
{
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem) PURE;

    STDMETHOD_(GEKRESOURCEID, LoadTexture)              (THIS_ LPCWSTR pFileName) PURE;

    STDMETHOD(CreateRenderStates)                       (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates, IUnknown **ppStates) PURE;
    STDMETHOD(CreateDepthStates)                        (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates, IUnknown **ppStates) PURE;
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates, IUnknown **ppStates) PURE;
    STDMETHOD(CreateBlendStates)                        (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates, IUnknown **ppStates) PURE;

    STDMETHOD(CompileComputeProgram)                    (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(CompileVertexProgram)                     (THIS_ LPCSTR pProgram, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(CompileGeometryProgram)                   (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(CompilePixelProgram)                      (THIS_ LPCSTR pProgram, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(LoadComputeProgram)                       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(LoadVertexProgram)                        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(LoadGeometryProgram)                      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD(LoadPixelProgram)                         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, IUnknown **ppProgram, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;

    STDMETHOD(CreateTexture)                            (THIS_ UINT32 nXSize, UINT32 nYSize, UINT32 nZSize, GEK3DVIDEO::DATA::FORMAT eFormat, UINT32 nFlags, IGEK3DVideoTexture **ppTexture) PURE;
    STDMETHOD_(void, UpdateTexture)                     (THIS_ IGEK3DVideoTexture *pTexture, void *pBuffer, UINT32 nPitch, trect<UINT32> *pDestRect = nullptr) PURE;
    STDMETHOD(LoadTexture)                              (THIS_ LPCWSTR pFileName, UINT32 nFlags, IGEK3DVideoTexture **ppTexture) PURE;
    STDMETHOD(CreateSamplerStates)                      (THIS_ const GEK3DVIDEO::SAMPLERSTATES &kStates, IUnknown **ppStates) PURE;
};
