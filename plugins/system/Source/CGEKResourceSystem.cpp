#include "CGEKResourceSystem.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKResourceSystem);

void CGEKResourceSystem::OnLoadTexture(CStringW strFileName, UINT32 nFlags, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnLoadComputeProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnLoadVertexProgram(CStringW strFileName, CStringA strEntry, std::vector<GEK3DVIDEO::INPUTELEMENT> aLayout, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnLoadGeometryProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnLoadPixelProgram(CStringW strFileName, CStringA strEntry, std::unordered_map<CStringA, CStringA> aDefines, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnCreateRenderStates(GEK3DVIDEO::RENDERSTATES kStates, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnCreateDepthStates(GEK3DVIDEO::DEPTHSTATES kStates, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnCreateUnifiedBlendStates(GEK3DVIDEO::UNIFIEDBLENDSTATES kStates, GEKRESOURCEID nResourceID)
{
}

void CGEKResourceSystem::OnCreateIndependentBlendStates(GEK3DVIDEO::INDEPENDENTBLENDSTATES kStates, GEKRESOURCEID nResourceID)
{
}

CGEKResourceSystem::CGEKResourceSystem(void)
    : m_pVideoSystem(nullptr)
    , m_nNextResourceID(GEKINVALIDRESOURCEID)
{
}

CGEKResourceSystem::~CGEKResourceSystem(void)
{
}

STDMETHODIMP_(void) CGEKResourceSystem::Destroy(void)
{
    Flush();
}

STDMETHODIMP CGEKResourceSystem::Initialize(IGEK3DVideoSystem *pVideoSystem)
{
    REQUIRE_RETURN(pVideoSystem, E_INVALIDARG);

    m_pVideoSystem = pVideoSystem;

    return S_OK;
}

STDMETHODIMP_(void) CGEKResourceSystem::Flush(void)
{
    m_aTasks.cancel();
    m_aTasks.wait();
    m_aQueue.clear();
    m_aResources.clear();
    m_aNames.clear();
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadTexture(LPCWSTR pFileName, UINT32 nFlags)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        nResourceID = InterlockedIncrement(&m_nNextResourceID);
        m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadTexture, this, pFileName, nFlags, nResourceID));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKResourceSystem::SetResource(IGEK3DVideoContextSystem *pSystem, UINT32 nIndex, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pSystem->SetResource(nIndex, (*pIterator).second);
    }
}

STDMETHODIMP_(void) CGEKResourceSystem::SetUnorderedAccess(IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pSystem->SetUnorderedAccess(nStage, (*pIterator).second);
    }
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadComputeProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        std::unordered_map<CStringA, CStringA> aDefines;
        if (pDefines)
        {
            aDefines = (*pDefines);
        }

        m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadComputeProgram, this, pFileName, pEntry, aDefines));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadVertexProgram(LPCWSTR pFileName, LPCSTR pEntry, const std::vector<GEK3DVIDEO::INPUTELEMENT> &aLayout, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        std::unordered_map<CStringA, CStringA> aDefines;
        if (pDefines)
        {
            aDefines = (*pDefines);
        }

        m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadVertexProgram, this, pFileName, pEntry, aDefines));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadGeometryProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        std::unordered_map<CStringA, CStringA> aDefines;
        if (pDefines)
        {
            aDefines = (*pDefines);
        }

        m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadGeometryProgram, this, pFileName, pEntry, aDefines));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadPixelProgram(LPCWSTR pFileName, LPCSTR pEntry, std::unordered_map<CStringA, CStringA> *pDefines)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        std::unordered_map<CStringA, CStringA> aDefines;
        if (pDefines)
        {
            aDefines = (*pDefines);
        }

        m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadPixelProgram, this, pFileName, pEntry, aDefines));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKResourceSystem::SetProgram(IGEK3DVideoContextSystem *pSystem, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pSystem->SetProgram((*pIterator).second);
    }
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::CreateRenderStates(const GEK3DVIDEO::RENDERSTATES &kStates)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        m_aQueue.push(std::bind(&CGEKResourceSystem::OnCreateRenderStates, this, kStates));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::CreateDepthStates(const GEK3DVIDEO::DEPTHSTATES &kStates)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        m_aQueue.push(std::bind(&CGEKResourceSystem::OnCreateDepthStates, this, kStates));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::CreateBlendStates(const GEK3DVIDEO::UNIFIEDBLENDSTATES &kStates)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        m_aQueue.push(std::bind(&CGEKResourceSystem::OnCreateUnifiedBlendStates, this, kStates));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::CreateBlendStates(const GEK3DVIDEO::INDEPENDENTBLENDSTATES &kStates)
{
    GEKRESOURCEID nResourceID = GEKINVALIDRESOURCEID;
    auto pIterator = m_aNames.find(pFileName);
    if (pIterator == m_aNames.end())
    {
        m_aQueue.push(std::bind(&CGEKResourceSystem::OnCreateIndependentBlendStates, this, kStates));
        m_aTasks.run([&](void) -> void
        {
            std::function<void(void)> Function;
            if (m_aQueue.try_pop(Function))
            {
                Function();
            }
        });
    }
    else
    {
        nResourceID = (*pIterator).second;
    }

    return nResourceID;
}

STDMETHODIMP_(void) CGEKResourceSystem::SetRenderStates(IGEK3DVideoContext *pContext, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pContext->SetRenderStates((*pIterator).second);
    }
}

STDMETHODIMP_(void) CGEKResourceSystem::SetDepthStates(IGEK3DVideoContext *pContext, UINT32 nStencilReference, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pContext->SetDepthStates(nStencilReference, (*pIterator).second);
    }
}

STDMETHODIMP_(void) CGEKResourceSystem::SetBlendStates(IGEK3DVideoContext *pContext, const float4 &nBlendFactor, UINT32 nMask, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pContext->SetBlendStates(nBlendFactor, nMask, (*pIterator).second);
    }
}

STDMETHODIMP_(void) CGEKResourceSystem::SetSamplerStates(IGEK3DVideoContextSystem *pSystem, UINT32 nStage, const GEKRESOURCEID &nResourceID)
{
    auto pIterator = m_aResources.find(nResourceID);
    if (pIterator != m_aResources.end())
    {
        pSystem->SetSamplerStates(nStage, (*pIterator).second);
    }
}

STDMETHODIMP_(void) CGEKResourceSystem::OnResizeBegin(void)
{
}

STDMETHODIMP CGEKResourceSystem::OnResizeEnd(UINT32 nXSize, UINT32 nYSize, bool bWindowed)
{
    return S_OK;
}
