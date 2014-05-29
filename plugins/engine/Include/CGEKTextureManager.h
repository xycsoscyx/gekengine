#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"

class CGEKTextureManager : public CGEKUnknown
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::map<GEKHASH, CComPtr<IUnknown>> m_aTextures;

public:
    CGEKTextureManager(void);
    virtual ~CGEKTextureManager(void);
    DECLARE_UNKNOWN(CGEKTextureManager);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);
};