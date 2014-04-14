#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKStaticModel : public CGEKUnknown
                      , public CGEKVideoSystemUser
                      , public CGEKProgramManagerUser
                      , public CGEKMaterialManagerUser
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
    aabb m_nAABB;
    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IGEKVideoVertexBuffer> m_spPositionBuffer;
    CComPtr<IGEKVideoVertexBuffer> m_spTexCoordBuffer;
    CComPtr<IGEKVideoVertexBuffer> m_spBasisBuffer;
    CComPtr<IGEKVideoIndexBuffer> m_spIndexBuffer;
    std::map<CComPtr<IUnknown>, MATERIAL> m_aMaterials;
    CComPtr<IGEKVideoVertexBuffer> m_spInstanceBuffer;

public:
    CGEKStaticModel(void);
    virtual ~CGEKStaticModel(void);
    DECLARE_UNKNOWN(CGEKStaticModel);

    // IGEKModel
    STDMETHOD(Load)                 (THIS_ const UINT8 *pBuffer, LPCWSTR pParams);
    STDMETHOD_(aabb, GetAABB)       (THIS);
    STDMETHOD_(void, Prepare)       (THIS);
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes, const std::vector<float4x4> &aInstances);
};