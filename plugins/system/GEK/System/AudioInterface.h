#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Math\Matrix4x4.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    namespace Audio
    {
        DECLARE_INTERFACE_IID(Class, "525E4F27-8A87-409C-A25F-8740393A4B7B");

        namespace Sample
        {
            DECLARE_INTERFACE_IID(Interface, "35560CF2-6972-44A3-9489-9CA0A5FE95C9") : virtual public IUnknown
            {
                STDMETHOD_(LPVOID, getBuffer)       (THIS) PURE;
                STDMETHOD_(void, setFrequency)      (THIS_ UINT32 frequency) PURE;
                STDMETHOD_(void, setVolume)         (THIS_ float volume) PURE;
            };
        }; // namespace Sample

        namespace Effect
        {
            DECLARE_INTERFACE_IID(Interface, "19ED8F1F-D117-4D9A-9AC0-7DC229D478D6") : virtual public IUnknown
            {
                STDMETHOD_(void, setPan)            (THIS_ float pan) PURE;
                STDMETHOD_(void, play)              (THIS_ bool loop) PURE;
            };
        }; // namespace Effect

        namespace Sound
        {
            DECLARE_INTERFACE_IID(Interface, "7C3C561D-669B-4559-A1DD-6350AE7A14C0") : virtual public IUnknown
            {
                STDMETHOD_(void, setDistance)       (THIS_ float minimum, float maximum) PURE;
                STDMETHOD_(void, play)              (THIS_ const Gek::Math::Float3 &origin, bool loop) PURE;
            };
        }; // namespace Sound

        DECLARE_INTERFACE_IID(Interface, "E760C91D-7AF9-4AAA-B8E5-08F8F9A23CEB") : virtual public IUnknown
        {
            STDMETHOD(initialize)               (THIS_ HWND window) PURE;

            STDMETHOD_(void, setMasterVolume)   (THIS_ float volume) PURE;
            STDMETHOD_(float, getMasterVolume)  (THIS) PURE;

            STDMETHOD_(void, setListener)       (THIS_ const Gek::Math::Float4x4 &matrix) PURE;
            STDMETHOD_(void, setDistanceFactor) (THIS_ float factor) PURE;
            STDMETHOD_(void, setDopplerFactor)  (THIS_ float factor) PURE;
            STDMETHOD_(void, setRollOffFactor)  (THIS_ float factor) PURE;

            STDMETHOD(copyEffect)               (THIS_ Effect::Interface **returnObject, Effect::Interface *source) PURE;
            STDMETHOD(copySound)                (THIS_ Sound::Interface **returnObject, Sound::Interface *source) PURE;

            STDMETHOD(loadEffect)               (THIS_ Effect::Interface **returnObject, LPCWSTR fileName) PURE;
            STDMETHOD(loadSound)                (THIS_ Sound::Interface **returnObject, LPCWSTR fileName) PURE;
        };
    }; // namespace Audio
}; // namespace Gek
