#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKStaticFactory.h"

class CGEKStaticModel : public CGEKUnknown
                      , public IGEKResource
                      , public IGEKModel
{
private:
    struct MATERIAL
    {
        UINT32 m_nFirstVertex;
        UINT32 m_nFirstIndex;
        UINT32 m_nNumIndices;
    };

private:
    IGEKVideoSystem *m_pVideoSystem;
    IGEKMaterialManager *m_pMaterialManager;
    IGEKProgramManager *m_pProgramManager;

    aabb m_nAABB;
    CComPtr<IGEKVideoBuffer> m_spPositionBuffer;
    CComPtr<IGEKVideoBuffer> m_spTexCoordBuffer;
    CComPtr<IGEKVideoBuffer> m_spBasisBuffer;
    CComPtr<IGEKVideoBuffer> m_spIndexBuffer;
    std::multimap<CComPtr<IUnknown>, MATERIAL> m_aMaterials;

public:
    CGEKStaticModel(void);
    virtual ~CGEKStaticModel(void);
    DECLARE_UNKNOWN(CGEKStaticModel);

    // IGEKUnknown
    STDMETHOD(Initialize)           (THIS);

    // IGEKResource
    STDMETHOD(Load)                 (THIS_ const UINT8 *pBuffer, LPCWSTR pParams);

    // IGEKModel
    STDMETHOD_(aabb, GetAABB)       (THIS);
    STDMETHOD_(void, Prepare)       (THIS);
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes, const std::vector<IGEKModel::INSTANCE> &aInstances);
};