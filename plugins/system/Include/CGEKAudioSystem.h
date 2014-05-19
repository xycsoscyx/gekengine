#pragma once

#define DIRECTSOUND_VERSION 0x0800

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKSystem.h"
#include <mmsystem.h>
#include <dsound.h>

class CGEKAudioSystem : public CGEKUnknown
                      , public CGEKContextUser
                      , public CGEKSystemUser
                      , public IGEKContextObserver
                      , public IGEKAudioSystem
{
private:
    UINT32 m_pContextHandle;
    CComQIPtr<IDirectSound8, &IID_IDirectSound8> m_spDirectSound;
    CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> m_spListener;
    CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> m_spPrimary;

private:
    HRESULT LoadFromFile(LPCWSTR pFileName, DWORD nFlags, GUID nAlgorithm, IDirectSoundBuffer **ppBuffer);

public:
    CGEKAudioSystem(void);
    virtual ~CGEKAudioSystem(void);
    DECLARE_UNKNOWN(CGEKAudioSystem)

    // IGEKContextObserver
    STDMETHOD(OnRegistration)           (THIS_ IUnknown *pObject);

    // IGEKUnknown
    STDMETHOD(Initialize)               (THIS);

    // IGEKAudioSystem
    STDMETHOD_(void, SetMasterVolume)   (THIS_ float nVolume);
    STDMETHOD_(float, GetMasterVolume)  (THIS);
    STDMETHOD_(void, SetListener)       (THIS_ const float4x4 &nMatrix);
    STDMETHOD_(void, SetDistanceFactor) (THIS_ float nFactor);
    STDMETHOD_(void, SetDopplerFactor)  (THIS_ float nFactor);
    STDMETHOD_(void, SetRollOffFactor)  (THIS_ float nFactor);
    STDMETHOD(CopyEffect)               (THIS_ IGEKAudioEffect *pSource, IGEKAudioEffect **ppEffect);
    STDMETHOD(CopySound)                (THIS_ IGEKAudioSound *pSource, IGEKAudioSound **ppSound);
    STDMETHOD(LoadEffect)               (THIS_ LPCWSTR pFileName, IGEKAudioEffect **ppEffect);
    STDMETHOD(LoadSound)                (THIS_ LPCWSTR pFileName, IGEKAudioSound **ppSound);
};
