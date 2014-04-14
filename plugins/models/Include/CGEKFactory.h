#pragma once

#include "GEKContext.h"
#include "GEKSystem.h"
#include "GEKAPI.h"

class CGEKFactory : public CGEKUnknown
                 , public CGEKContextUser
                 , public IGEKFactory
{
public:
    CGEKFactory(void);
    virtual ~CGEKFactory(void);
    DECLARE_UNKNOWN(CGEKFactory);

    // IGEKFactory
    STDMETHOD(Create)               (THIS_ const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject);
};