#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKStaticWorld : public CGEKUnknown
                      , public CGEKObservable
                      , public CGEKContextUser
                      , public CGEKVideoSystemUser
                      , public CGEKProgramManagerUser
                      , public CGEKMaterialManagerUser
                      , public IGEKWorld
{
private:
    struct MATERIAL
    {
        UINT32 m_nFirstVertex;
        UINT32 m_nFirstIndex;
        UINT32 m_nNumIndices;
    };

    struct PORTAL
    {
        UINT32 m_nFirstEdge;
        UINT32 m_nNumEdges;
        INT32 m_nPositiveArea;
        INT32 m_nNegativeArea;
    };

    struct AREA
    {
        UINT32 m_nFrame;
        std::multimap<CComPtr<IUnknown>, MATERIAL> m_aMaterials;
        std::vector<PORTAL *> m_aPortals;
    };

    struct NODE : public plane
    {
        INT32 m_nPositiveChild;
        INT32 m_nNegativeChild;
    };

private:
    CComPtr<IUnknown> m_spVertexProgram;
    CComPtr<IGEKVideoVertexBuffer> m_spPositionBuffer;
    CComPtr<IGEKVideoVertexBuffer> m_spTexCoordBuffer;
    CComPtr<IGEKVideoVertexBuffer> m_spBasisBuffer;
    CComPtr<IGEKVideoIndexBuffer> m_spIndexBuffer;

    aabb m_nAABB;
    std::vector<AREA> m_aAreas;
    std::vector<float3> m_aPortalEdges;
    std::vector<PORTAL> m_aPortals;
    std::vector<NODE> m_aNodes;

    int m_nCurrentArea;
    UINT32 m_nFrame;

private:
    int GetArea(const float3 &nPoint);

public:
    CGEKStaticWorld(void);
    virtual ~CGEKStaticWorld(void);
    DECLARE_UNKNOWN(CGEKStaticWorld);

    // IGEKWorld
    STDMETHOD(Load)                 (THIS_ const UINT8 *pBuffer, std::function<HRESULT(float3 *, IUnknown *)> OnStaticFace);
    STDMETHOD_(aabb, GetAABB)       (THIS);
    STDMETHOD_(void, Prepare)       (THIS_ const frustum &nFrustum);
    STDMETHOD_(bool, IsVisible)     (THIS_ const aabb &nBox);
    STDMETHOD_(bool, IsVisible)     (THIS_ const obb &nBox);
    STDMETHOD_(bool, IsVisible)     (THIS_ const sphere &nSphere);
    STDMETHOD_(void, Draw)          (THIS_ UINT32 nVertexAttributes);
};