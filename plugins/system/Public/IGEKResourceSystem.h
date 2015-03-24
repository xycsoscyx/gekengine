#pragma once

#include "GEKContext.h"

DECLARE_INTERFACE(IGEK3DVideoSystem);
DECLARE_INTERFACE(IGEK3DVideoContext);
DECLARE_INTERFACE(IGEK3DVideoContextSystem);

typedef UINT32 GEKRESOURCEID;
const GEKRESOURCEID GEKINVALIDRESOURCEID = 0;

DECLARE_INTERFACE_IID_(IGEKResourceSystem, IUnknown, "4680DA6E-2FD2-4918-A635-497C09A3969E")
{
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem) PURE;

    STDMETHOD_(GEKRESOURCEID, LoadTexture)              (THIS_ LPCWSTR pFileName, UINT32 nFlags) PURE;
    STDMETHOD_(void, SetResource)                       (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nIndex, const GEKRESOURCEID &nResourceID) PURE;
    STDMETHOD_(void, SetUnorderedAccess)                (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID) PURE;

    STDMETHOD_(GEKRESOURCEID, LoadComputeProgram)       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKRESOURCEID, LoadVertexProgram)        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKRESOURCEID, LoadGeometryProgram)      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(GEKRESOURCEID, LoadPixelProgram)         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr) PURE;
    STDMETHOD_(void, SetProgram)                        (THIS_ IGEK3DVideoContextSystem *pSystem, const GEKRESOURCEID &nResourceID) PURE;

    STDMETHOD_(GEKRESOURCEID, CreateRenderStates)       (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates) PURE;
    STDMETHOD_(GEKRESOURCEID, CreateDepthStates)        (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates) PURE;
    STDMETHOD_(GEKRESOURCEID, CreateBlendStates)        (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates) PURE;
    STDMETHOD_(GEKRESOURCEID, CreateBlendStates)        (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates) PURE;
    STDMETHOD_(void, SetRenderStates)                   (THIS_ IGEK3DVideoContext *pContext, const GEKRESOURCEID &nResourceID) PURE;
    STDMETHOD_(void, SetDepthStates)                    (THIS_ IGEK3DVideoContext *pContext, UINT32 nStencilReference, const GEKRESOURCEID &nResourceID) PURE;
    STDMETHOD_(void, SetBlendStates)                    (THIS_ IGEK3DVideoContext *pContext, const float4 &nBlendFactor, UINT32 nMask, const GEKRESOURCEID &nResourceID) PURE;
    STDMETHOD_(void, SetSamplerStates)                  (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID) PURE;
};
