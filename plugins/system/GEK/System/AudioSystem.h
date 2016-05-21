#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\Trace.h"
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    namespace Audio
    {
        GEK_BASE_EXCEPTION();
    }; // namespace Audio

    interface AudioSample
    {
        void * const getBuffer(void);
        void setFrequency(UINT32 frequency);
        void setVolume(float volume);
    };

    interface AudioEffect : public AudioSample
    {
        void setPan(float pan);
        void play(bool loop);
    };

    interface AudioSound : public AudioSample
    {
        void setDistance(float minimum, float maximum);
        void play(const Gek::Math::Float3 &origin, bool loop);
    };

    interface AudioSystem
    {
        void initialize(HWND window);

        void setMasterVolume(float volume);
        float getMasterVolume(void);

        void setListener(const Gek::Math::Float4x4 &matrix);
        void setDistanceFactor(float factor);
        void setDopplerFactor(float factor);
        void setRollOffFacto  (float factor);

        std::shared_ptr<AudioEffect> copyEffect(AudioEffect *source);
        std::shared_ptr<AudioSound> copySound(AudioSound *source);

        std::shared_ptr<AudioEffect> loadEffect(LPCWSTR fileName);
        std::shared_ptr<AudioSound> loadSound(LPCWSTR fileName);
    };

    DECLARE_INTERFACE_IID(AudioSystemRegistration, "525E4F27-8A87-409C-A25F-8740393A4B7B");
}; // namespace Gek
