#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"

class CGEKResourceSystem : public CGEKUnknown
                         , public IGEKResourceSystem
                         , public IGEK3DVideoObserver
{
private:
    IGEK3DVideoSystem *m_pVideoSystem;

public:
    CGEKResourceSystem(void);
    virtual ~CGEKResourceSystem(void);
    DECLARE_UNKNOWN(CGEKResourceSystem);

    // IGEKResourceSystem
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem);
};
