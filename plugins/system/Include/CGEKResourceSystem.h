#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <ppl.h>

class CGEKResourceSystem : public CGEKUnknown
                         , public IGEKResourceSystem
                         , public IGEK3DVideoObserver
{
private:
    IGEK3DVideoSystem *m_pVideoSystem;

    concurrency::task_group m_aTasks;
    concurrency::concurrent_queue<std::function<void(void)>> m_aQueue;

    GEKRESOURCEID m_nNextResourceID;
    concurrency::concurrent_unordered_map<GEKRESOURCEID, CComPtr<IUnknown>> m_aResources;
    concurrency::concurrent_unordered_map<CStringW, GEKRESOURCEID> m_aNames;

private:
    void OnLoadTexture(CStringW strFileName, UINT32 nFlags, GEKRESOURCEID nResourceID);
    void OnLoadComputeProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID);
    void OnLoadVertexProgram(CStringW strFileName, CStringA strEntry, std::vector<GEK3DVIDEO::INPUTELEMENT> aLayout, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID);
    void OnLoadGeometryProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID);
    void OnLoadPixelProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID);
    void OnCreateRenderStates(GEK3DVIDEO::RENDERSTATES kStates, GEKRESOURCEID nResourceID);
    void OnCreateDepthStates(GEK3DVIDEO::DEPTHSTATES kStates, GEKRESOURCEID nResourceID);
    void OnCreateUnifiedBlendStates(GEK3DVIDEO::UNIFIEDBLENDSTATES kStates, GEKRESOURCEID nResourceID);
    void OnCreateIndependentBlendStates(GEK3DVIDEO::INDEPENDENTBLENDSTATES kStates, GEKRESOURCEID nResourceID);

public:
    CGEKResourceSystem(void);
    virtual ~CGEKResourceSystem(void);
    DECLARE_UNKNOWN(CGEKResourceSystem);

    // IGEKUnknown
    STDMETHOD_(void, Destroy)                           (THIS);

    // IGEKResourceSystem
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem);
    STDMETHOD_(void, Flush)                             (THIS);
    STDMETHOD_(GEKRESOURCEID, LoadTexture)              (THIS_ LPCWSTR pFileName, UINT32 nFlags);
    STDMETHOD_(void, SetResource)                       (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nIndex, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(void, SetUnorderedAccess)                (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(GEKRESOURCEID, LoadComputeProgram)       (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKRESOURCEID, LoadVertexProgram)        (THIS_ LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKRESOURCEID, LoadGeometryProgram)      (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(GEKRESOURCEID, LoadPixelProgram)         (THIS_ LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines = nullptr);
    STDMETHOD_(void, SetProgram)                        (THIS_ IGEK3DVideoContextSystem *pSystem, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(GEKRESOURCEID, CreateRenderStates)       (THIS_ const GEK3DVIDEO::RENDERSTATES &kStates);
    STDMETHOD_(GEKRESOURCEID, CreateDepthStates)        (THIS_ const GEK3DVIDEO::DEPTHSTATES &kStates);
    STDMETHOD_(GEKRESOURCEID, CreateBlendStates)        (THIS_ const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates);
    STDMETHOD_(GEKRESOURCEID, CreateBlendStates)        (THIS_ const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates);
    STDMETHOD_(void, SetRenderStates)                   (THIS_ IGEK3DVideoContext *pContext, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(void, SetDepthStates)                    (THIS_ IGEK3DVideoContext *pContext, UINT32 nStencilReference, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(void, SetBlendStates)                    (THIS_ IGEK3DVideoContext *pContext, const float4 &nBlendFactor, UINT32 nMask, const GEKRESOURCEID &nResourceID);
    STDMETHOD_(void, SetSamplerStates)                  (THIS_ IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID);

    // IGEK3DVideoObserver
    STDMETHOD_(void, OnResizeBegin)                     (THIS);
    STDMETHOD(OnResizeEnd)                              (THIS_ UINT32 nXSize, UINT32 nYSize, bool bWindowed);
};