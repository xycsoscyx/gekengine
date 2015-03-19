#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
#include <ppl.h>

class CGEKResourceSystem : public CGEKUnknown
                         , public CGEKObservable
                         , public IGEKResourceSystem
                         , public IGEK3DVideoObserver
{
private:
    IGEK3DVideoSystem *m_pVideoSystem;

    concurrency::task_group m_aTasks;
    concurrency::concurrent_queue<std::function<void(void)>> m_aQueue;

    GEKRESOURCEID m_nNextResourceID;
    concurrency::concurrent_unordered_map<GEKRESOURCEID, CComPtr<IUnknown>> m_aResources;

private:
    void LoadTexture2(CStringW strFileName, UINT32 nFlags, GEKRESOURCEID nResourceID);

public:
    CGEKResourceSystem(void);
    virtual ~CGEKResourceSystem(void);
    DECLARE_UNKNOWN(CGEKResourceSystem);

    // IGEKUnknown
    STDMETHOD_(void, Destroy)                           (THIS);

    // IGEKResourceSystem
    STDMETHOD(Initialize)                               (THIS_ IGEK3DVideoSystem *pVideoSystem);
    STDMETHOD_(GEKRESOURCEID, LoadTexture)              (THIS_ LPCWSTR pFileName, UINT32 nFlags);

    // IGEK3DVideoObserver
    STDMETHOD_(void, OnResizeBegin)                     (THIS);
    STDMETHOD(OnResizeEnd)                              (THIS_ UINT32 nXSize, UINT32 nYSize, bool bWindowed);
};