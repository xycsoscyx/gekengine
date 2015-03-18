#include "CGEKResourceSystem.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"

GEKRESOURCEID gs_nNextResourceID = GEKINVALIDRESOURCEID;

BEGIN_INTERFACE_LIST(CGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEKResourceSystem)
    INTERFACE_LIST_ENTRY_COM(IGEK3DVideoObserver)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKResourceSystem);

CGEKResourceSystem::CGEKResourceSystem(void)
    : m_pVideoSystem(nullptr)
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

STDMETHODIMP_(void) CGEKResourceSystem::OnResizeBegin(void)
{
}

STDMETHODIMP CGEKResourceSystem::OnResizeEnd(UINT32 nXSize, UINT32 nYSize, bool bWindowed)
{
    return S_OK;
}
