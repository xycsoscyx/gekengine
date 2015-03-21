#include "CGEKResourceSystem.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKResourceSystem);

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
    m_aTasks.cancel();
    m_aTasks.wait();
}

STDMETHODIMP CGEKResourceSystem::Initialize(IGEK3DVideoSystem *pVideoSystem)
{
    REQUIRE_RETURN(pVideoSystem, E_INVALIDARG);

    m_pVideoSystem = pVideoSystem;

    return S_OK;
}

void CGEKResourceSystem::OnLoadTexture(CStringW strFileName, UINT32 nFlags, GEKRESOURCEID nResourceID)
{
    CComPtr<IGEK3DVideoTexture> spTexture;
    m_pVideoSystem->LoadTexture(strFileName, nFlags, &spTexture);
    if (spTexture)
    {
        m_aResources[nResourceID] = spTexture;
    }
}

STDMETHODIMP_(GEKRESOURCEID) CGEKResourceSystem::LoadTexture(LPCWSTR pFileName, UINT32 nFlags)
{
    GEKRESOURCEID nResourceID = InterlockedIncrement(&m_nNextResourceID);

    m_aQueue.push(std::bind(&CGEKResourceSystem::OnLoadTexture, this, pFileName, nFlags, nResourceID));
    m_aTasks.run([&](void) -> void
    {
        std::function<void(void)> Function;
        if (m_aQueue.try_pop(Function))
        {
            Function();
        }
    });

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

STDMETHODIMP_(void) CGEKResourceSystem::OnResizeBegin(void)
{
}

STDMETHODIMP CGEKResourceSystem::OnResizeEnd(UINT32 nXSize, UINT32 nYSize, bool bWindowed)
{
    return S_OK;
}
