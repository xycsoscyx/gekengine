#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <ppl.h>

class CGEKResourceSystem : public CGEKUnknown
    , public IGEKResourceSystem
    , public IGEK3DVideoObserver
{
private:
    IGEK3DVideoSystem *m_pVideoSystem;
    concurrency::task_group m_aTasks;

public:
    CGEKResourceSystem(void);
    virtual ~CGEKResourceSystem(void);
    DECLARE_UNKNOWN(CGEKResourceSystem);

    // IGEKUnknown
    STDMETHOD_(void, Destroy)                   (THIS);

    // IGEKResourceSystem
    STDMETHOD(Initialize)                       (THIS_ IGEK3DVideoSystem *pVideoSystem);

    // IGEK3DVideoObserver
    STDMETHOD_(void, OnResizeBegin)             (THIS);
    STDMETHOD(OnResizeEnd)                      (THIS_ UINT32 nXSize, UINT32 nYSize, bool bWindowed);
};