#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKStaticProvider.h"

class CGEKStaticProvider : public CGEKUnknown
                         , public IGEKResourceProvider
                         , public IGEKStaticProvider
{
private:
    UINT32 m_nNumInstances;
    CComPtr<IGEKVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;

public:
    CGEKStaticProvider(void);
    virtual ~CGEKStaticProvider(void);
    DECLARE_UNKNOWN(CGEKStaticProvider);

    // IGEKUnknown
    STDMETHOD(Initialize)                                       (THIS);
    STDMETHOD_(void, Destroy)                                   (THIS);

    // IGEKResourceProvider
    STDMETHOD(Load)                                             (THIS_ LPCWSTR pName, const UINT8 *pBuffer, UINT32 nBufferSize, IUnknown **ppObject);

    // IGEKStaticProvider
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS);
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS);
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS);
};