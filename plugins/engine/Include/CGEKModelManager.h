#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKModelManager : public CGEKUnknown
{
private:
    IGEKSystem *m_pSystem;
    IGEKVideoSystem *m_pVideoSystem;

    std::list<CComPtr<IGEKFactory>> m_aFactories;

    std::map<GEKHASH, CComPtr<IUnknown>> m_aModels;

public:
    CGEKModelManager(void);
    virtual ~CGEKModelManager(void);
    DECLARE_UNKNOWN(CGEKModelManager);

    // IGEKUnknown
    STDMETHOD(Initialize)                   (THIS);
    STDMETHOD_(void, Destroy)               (THIS);

    // IGEKResourceProvider
    STDMETHOD(Load)                         (THIS_ const UINT8 *pBuffer, LPCWSTR pParams, IUnknown **ppObject);
};