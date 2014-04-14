#pragma once

#include "GEKMath.h"
#include "GEKContext.h"
#include <atlbase.h>
#include <atlstr.h>

DECLARE_INTERFACE_IID_(IGEKAudioSample, IUnknown, "35560CF2-6972-44A3-9489-9CA0A5FE95C9")
{
    STDMETHOD_(void *, GetBuffer)       (THIS) PURE;
    STDMETHOD_(void, SetFrequency)      (THIS_ UINT32 nFrequency) PURE;
    STDMETHOD_(void, SetVolume)         (THIS_ float nVolume) PURE;
};

DECLARE_INTERFACE_IID_(IGEKAudioEffect, IUnknown, "19ED8F1F-D117-4D9A-9AC0-7DC229D478D6")
{
    STDMETHOD_(void, SetPan)            (THIS_ float fPan) PURE;
    STDMETHOD_(void, Play)              (THIS_ bool bLoop) PURE;
};

DECLARE_INTERFACE_IID_(IGEKAudioSound, IUnknown, "7C3C561D-669B-4559-A1DD-6350AE7A14C0")
{
    STDMETHOD_(void, SetDistance)       (THIS_ float nMin, float nMax) PURE;
    STDMETHOD_(void, Play)              (THIS_ const float3 &kOrigin, bool bLoop) PURE;
};

DECLARE_INTERFACE_IID_(IGEKAudioSystem, IUnknown, "E760C91D-7AF9-4AAA-B8E5-08F8F9A23CEB")
{
    STDMETHOD_(void, SetMasterVolume)   (THIS_ float nVolume) PURE;
    STDMETHOD_(float, GetMasterVolume)  (THIS) PURE;

    STDMETHOD_(void, SetListener)       (THIS_ const float4x4 &nMatrix) PURE;
    STDMETHOD_(void, SetDistanceFactor) (THIS_ float nFactor) PURE;
    STDMETHOD_(void, SetDopplerFactor)  (THIS_ float nFactor) PURE;
    STDMETHOD_(void, SetRollOffFactor)  (THIS_ float nFactor) PURE;

    STDMETHOD(CopyEffect)               (THIS_ IGEKAudioEffect *pSource, IGEKAudioEffect **ppEffect) PURE;
    STDMETHOD(CopySound)                (THIS_ IGEKAudioSound *pSource, IGEKAudioSound **ppSound) PURE;

    STDMETHOD(LoadEffect)               (THIS_ LPCWSTR pFileName, IGEKAudioEffect **ppEffect) PURE;
    STDMETHOD(LoadSound)                (THIS_ LPCWSTR pFileName, IGEKAudioSound **ppSound) PURE;
};

SYSTEM_USER(AudioSystem, "11D8675A-3EFB-4EE9-B864-B86C1470FE1D");
