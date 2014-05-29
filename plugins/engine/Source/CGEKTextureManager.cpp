#include "CGEKTextureManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKTextureManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKTextureManager)

CGEKTextureManager::CGEKTextureManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
{
}

CGEKTextureManager::~CGEKTextureManager(void)
{
}

STDMETHODIMP CGEKTextureManager::Initialize(void)
{
    GEKFUNCTION(nullptr);
    HRESULT hRetVal = E_FAIL;
    m_pSystem = GetContext()->GetCachedClass<IGEKSystem>(CLSID_GEKSystem);
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    if (m_pSystem != nullptr && m_pVideoSystem != nullptr)
    {
        hRetVal = S_OK;
    }

    return hRetVal;
}

STDMETHODIMP_(void) CGEKTextureManager::Destroy(void)
{
    m_aTextures.clear();
}
