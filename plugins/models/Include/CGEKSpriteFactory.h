#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"
#include "IGEKSpriteFactory.h"

class CGEKSpriteFactory : public CGEKUnknown
                        , public IGEKFactory
                        , public IGEKSpriteFactory
{
private:
    UINT32 m_nNumInstances;
    CComPtr<IGEKVideoBuffer> m_spInstanceBuffer;
    CComPtr<IUnknown> m_spVertexProgram;

public:
    CGEKSpriteFactory(void);
    virtual ~CGEKSpriteFactory(void);
    DECLARE_UNKNOWN(CGEKSpriteFactory);

    // IGEKUnknown
    STDMETHOD(Initialize)                                       (THIS);
    STDMETHOD_(void, Destroy)                                   (THIS);

    // IGEKFactory
    STDMETHOD(Create)                                           (THIS_ const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject);

    // IGEKSpriteFactory
    STDMETHOD_(IUnknown *, GetVertexProgram)                    (THIS);
    STDMETHOD_(IGEKVideoBuffer *, GetInstanceBuffer)            (THIS);
    STDMETHOD_(UINT32, GetNumInstances)                         (THIS);
};