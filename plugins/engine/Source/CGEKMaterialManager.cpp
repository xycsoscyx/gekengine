#include "CGEKMaterialManager.h"

#include "GEKEngineCLSIDs.h"
#include "GEKSystemCLSIDs.h"

BEGIN_INTERFACE_LIST(CGEKMaterialManager)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKMaterialManager)

CGEKMaterialManager::CGEKMaterialManager(void)
    : m_pSystem(nullptr)
    , m_pVideoSystem(nullptr)
{
}

CGEKMaterialManager::~CGEKMaterialManager(void)
{
}

STDMETHODIMP CGEKMaterialManager::Initialize(void)
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

STDMETHODIMP_(void) CGEKMaterialManager::Destroy(void)
{
    m_aMaterials.clear();
}
