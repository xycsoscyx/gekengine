#include "CGEKSpriteModel.h"
#include <algorithm>

#include "GEKSystemCLSIDs.h"
#include "GEKEngineCLSIDs.h"
#include "GEKModels.h"

BEGIN_INTERFACE_LIST(CGEKSpriteModel)
    INTERFACE_LIST_ENTRY_COM(IGEKResource)
    INTERFACE_LIST_ENTRY_COM(IGEKModel)
END_INTERFACE_LIST_UNKNOWN

REGISTER_CLASS(CGEKSpriteModel)

CGEKSpriteModel::CGEKSpriteModel(void)
    : m_pVideoSystem(nullptr)
    , m_pMaterialManager(nullptr)
    , m_pProgramManager(nullptr)
    , m_pSpriteFactory(nullptr)
{
}

CGEKSpriteModel::~CGEKSpriteModel(void)
{
}

STDMETHODIMP CGEKSpriteModel::Initialize(void)
{
    m_pVideoSystem = GetContext()->GetCachedClass<IGEKVideoSystem>(CLSID_GEKVideoSystem);
    m_pMaterialManager = GetContext()->GetCachedClass<IGEKMaterialManager>(CLSID_GEKRenderSystem);
    m_pProgramManager = GetContext()->GetCachedClass<IGEKProgramManager>(CLSID_GEKRenderSystem);
    m_pSpriteFactory = GetContext()->GetCachedClass<IGEKSpriteFactory>(CLSID_GEKSpriteFactory);
    return ((m_pVideoSystem && m_pMaterialManager && m_pProgramManager && m_pSpriteFactory) ? S_OK : E_FAIL);
}

STDMETHODIMP CGEKSpriteModel::Load(const UINT8 *pBuffer, LPCWSTR pName, LPCWSTR pParams)
{
    GEKFUNCTION(L"Name(%s), Params(%s)", pName, pParams);
    REQUIRE_RETURN(pBuffer, E_INVALIDARG);

    HRESULT hRetVal = E_INVALIDARG;

    return hRetVal;
}

STDMETHODIMP_(aabb) CGEKSpriteModel::GetAABB(void) const
{
    return m_nAABB;
}

STDMETHODIMP_(void) CGEKSpriteModel::Draw(UINT32 nVertexAttributes, const std::vector<IGEKModel::INSTANCE> &aInstances)
{
    if (!(nVertexAttributes & GEK_VERTEX_POSITION) &&
        !(nVertexAttributes & GEK_VERTEX_TEXCOORD) &&
        !(nVertexAttributes & GEK_VERTEX_BASIS))
    {
        return;
    }
}
