#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKStaticFactory.h"

class CGEKStaticCollision : public CGEKUnknown
                          , public IGEKResource
                          , public IGEKCollision
{
private:
    aabb m_nAABB;
    std::vector<float3> m_aVertices;
    std::vector<UINT16> m_aIndices;

public:
    CGEKStaticCollision(void);
    virtual ~CGEKStaticCollision(void);
    DECLARE_UNKNOWN(CGEKStaticCollision);

    // IGEKResource
    STDMETHOD(Load)                             (THIS_ const UINT8 *pBuffer, LPCWSTR pName, LPCWSTR pParams);

    // IGEKCollision
    STDMETHOD_(const UINT32, GetNumVertices)    (THIS) const;
    STDMETHOD_(const float3 *, GetVertices)     (THIS) const;
    STDMETHOD_(const UINT32, GetNumIndices)     (THIS) const;
    STDMETHOD_(const UINT16 *, GetIndices)      (THIS) const;
};