#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

DECLARE_INTERFACE(IGEKRenderFilter);
DECLARE_INTERFACE(IGEKMaterial);

class CGEKTextureProvider : public CGEKUnknown
                          , public IGEKResourceProvider
{
private:
    IGEKVideoSystem *m_pVideoSystem;

public:
    CGEKTextureProvider(void);
    virtual ~CGEKTextureProvider(void);
    DECLARE_UNKNOWN(CGEKTextureProvider);

    // IGEKResourceProvider
    STDMETHOD(Load)                         (THIS_ LPCWSTR pName, const UINT8 *pBuffer, UINT32 nBufferSize, IUnknown **ppObject);
};