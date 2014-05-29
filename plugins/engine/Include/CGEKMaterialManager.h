#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"

class CGEKMaterialManager : public CGEKUnknown
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;
    
    std::map<GEKHASH, CComPtr<IUnknown>> m_aMaterials;

public:
    CGEKMaterialManager(void);
    virtual ~CGEKMaterialManager(void);
    DECLARE_UNKNOWN(CGEKMaterialManager);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);
};