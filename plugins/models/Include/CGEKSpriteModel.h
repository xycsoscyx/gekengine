#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKSpriteFactory.h"

class CGEKSpriteModel : public CGEKUnknown
                      , public IGEKResource
                      , public IGEKModel
{
private:
    IGEKVideoSystem *m_pVideoSystem;
    IGEKMaterialManager *m_pMaterialManager;
    IGEKProgramManager *m_pProgramManager;
    IGEKSpriteFactory *m_pSpriteFactory;

    float2 m_nSize;
    CComPtr<IUnknown> m_spMaterial;

public:
    CGEKSpriteModel(void);
    virtual ~CGEKSpriteModel(void);
    DECLARE_UNKNOWN(CGEKSpriteModel);

    // IGEKUnknown
    STDMETHOD(Initialize)           (THIS);

    // IGEKResource
    STDMETHOD(Load)                 (THIS_ const UINT8 *pBuffer, LPCWSTR pName, LPCWSTR pParams);

    // IGEKModel
    STDMETHOD_(aabb, GetAABB)       (THIS) const;
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes, const std::vector<IGEKModel::INSTANCE> &aInstances);
};