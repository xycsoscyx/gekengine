#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Math\Matrix4x4.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    DECLARE_INTERFACE(AudioSample) : virtual public IUnknown
    {
        STDMETHOD_(LPVOID, getBuffer)       (THIS) PURE;
        STDMETHOD_(void, setFrequency)      (THIS_ UINT32 frequency) PURE;
        STDMETHOD_(void, setVolume)         (THIS_ float volume) PURE;
    };

    DECLARE_INTERFACE_IID(AudioEffect, "19ED8F1F-D117-4D9A-9AC0-7DC229D478D6") : virtual public AudioSample
    {
        STDMETHOD_(void, setPan)            (THIS_ float pan) PURE;
        STDMETHOD_(void, play)              (THIS_ bool loop) PURE;
    };

    DECLARE_INTERFACE_IID(AudioSound, "7C3C561D-669B-4559-A1DD-6350AE7A14C0") : virtual public AudioSample
    {
        STDMETHOD_(void, setDistance)       (THIS_ float minimum, float maximum) PURE;
        STDMETHOD_(void, play)              (THIS_ const Gek::Math::Float3 &origin, bool loop) PURE;
    };

    DECLARE_INTERFACE_IID(AudioSystem, "E760C91D-7AF9-4AAA-B8E5-08F8F9A23CEB") : virtual public IUnknown
    {
        STDMETHOD(initialize)               (THIS_ HWND window) PURE;

        STDMETHOD_(void, setMasterVolume)   (THIS_ float volume) PURE;
        STDMETHOD_(float, getMasterVolume)  (THIS) PURE;

        STDMETHOD_(void, setListener)       (THIS_ const Gek::Math::Float4x4 &matrix) PURE;
        STDMETHOD_(void, setDistanceFactor) (THIS_ float factor) PURE;
        STDMETHOD_(void, setDopplerFactor)  (THIS_ float factor) PURE;
        STDMETHOD_(void, setRollOffFactor)  (THIS_ float factor) PURE;

        STDMETHOD(copyEffect)               (THIS_ AudioEffect **returnObject, AudioEffect *source) PURE;
        STDMETHOD(copySound)                (THIS_ AudioSound **returnObject, AudioSound *source) PURE;

        STDMETHOD(loadEffect)               (THIS_ AudioEffect **returnObject, LPCWSTR fileName) PURE;
        STDMETHOD(loadSound)                (THIS_ AudioSound **returnObject, LPCWSTR fileName) PURE;
    };

    DECLARE_INTERFACE_IID(AudioSystemRegistration, "525E4F27-8A87-409C-A25F-8740393A4B7B");
}; // namespace Gek
