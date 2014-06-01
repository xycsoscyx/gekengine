#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKStaticFactory.h"

class CGEKFactory : public CGEKUnknown
                  , public IGEKFactory
                  , public IGEKStaticFactory
{
private:
    UINT32 m_nNumInstances;
    CComPtr<IGEKVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;

public:
    CGEKFactory(void);
    virtual ~CGEKFactory(void);
    DECLARE_UNKNOWN(CGEKFactory);

    // IGEKUnknown
    STDMETHOD(Initialize)                                       (THIS);
    STDMETHOD_(void, Destroy)                                   (THIS);

    // IGEKFactory
    STDMETHOD(Create)                                           (THIS_ const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject);

    // IGEKStaticFactory
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS);
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS);
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS);
};