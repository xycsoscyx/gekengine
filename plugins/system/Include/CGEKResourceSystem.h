#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <ppl.h>

template <class TYPE>
inline void hash_combine(std::size_t &nSeed, const TYPE &nValue)
{
    std::hash<TYPE> kHasher;
    nSeed ^= kHasher(nValue) + 0x9e3779b9 + (nSeed << 6) + (nSeed >> 2);
}

struct CGEKBlob
{
private:
    std::vector<UINT8> m_aData;
    std::size_t m_nHash;

public:
    CGEKBlob(const CStringW &strFileName, UINT32 nFlags)
        : m_nHash(0)
    {
        UINT32 nFileNameLength = (strFileName.GetLength() * sizeof(wchar_t));
        m_aData.resize(nFileNameLength + sizeof(DWORD));
        memcpy(m_aData.data(), strFileName.GetString(), nFileNameLength);
        memcpy((m_aData.data() + nFileNameLength), &nFlags, sizeof(UINT32));
        CalculateHash();
    }

    CGEKBlob(const CStringW &strFileName, const CStringA &strEntry)
        : m_nHash(0)
    {
        UINT32 nFileNameLength = (strFileName.GetLength() * sizeof(wchar_t));
        UINT32 nEntryLength = strEntry.GetLength();
        m_aData.resize(nFileNameLength + nEntryLength);
        memcpy(m_aData.data(), strFileName.GetString(), nFileNameLength);
        memcpy((m_aData.data() + nFileNameLength), strEntry.GetString(), nEntryLength);
        CalculateHash();
    }

    CGEKBlob(LPCVOID pData, size_t nSize)
        : m_nHash(0)
    {
        m_aData.resize(nSize);
        memcpy(m_aData.data(), pData, nSize);
        CalculateHash();
    }

    bool operator == (const CGEKBlob &kData) const
    {
        return (m_aData.size() == kData.m_aData.size() &&
            memcmp(m_aData.data(), kData.m_aData.data(), m_aData.size()) == 0);
    }

    void CalculateHash(void)
    {
        for (auto &nValue : m_aData)
        {
            hash_combine(m_nHash, nValue);
        }
    }

    std::size_t GetHash(void) const
    {
        return m_nHash;
    }
};

namespace std
{
    template <>
    struct hash<CGEKBlob> : public unary_function<CGEKBlob, size_t>
    {
        size_t operator()(const CGEKBlob &kBlob) const
        {
            return kBlob.GetHash();
        }
    };

    template <>
    struct equal_to<CGEKBlob> : public unary_function<CGEKBlob, bool>
    {
        bool operator()(const CGEKBlob &kBlobA, const CGEKBlob &kBlobB) const
        {
            return (kBlobA.GetHash() == kBlobB.GetHash());
        }
    };
}

class CGEKResourceSystem : public CGEKUnknown
                         , public IGEKResourceSystem
{
private:
    IGEK3DVideoSystem *m_pVideoSystem;

    concurrency::task_group m_aTasks;
    concurrency::concurrent_queue<std::function<void(void)>> m_aQueue;

    GEKRESOURCEID m_nNextResourceID;
    concurrency::concurrent_unordered_map<GEKRESOURCEID, CComPtr<IUnknown>> m_aResources;
    concurrency::concurrent_unordered_map<CGEKBlob, GEKRESOURCEID> m_aResourceMap;

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
};