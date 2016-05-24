#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\Trace.h"
#include "GEK\Context\Context.h"

namespace Gek
{
    namespace Audio
    {
        GEK_BASE_EXCEPTION();
    }; // namespace Audio

    GEK_INTERFACE(AudioSample)
    {
        virtual void * const getBuffer(void) = 0;
        virtual void setFrequency(UINT32 frequency) = 0;
        virtual void setVolume(float volume) = 0;
    };

    GEK_INTERFACE(AudioEffect)
        : public AudioSample
    {
        virtual void setPan(float pan) = 0;
        virtual void play(bool loop) = 0;
    };

    GEK_INTERFACE(AudioSound)
        : public AudioSample
    {
        virtual void setDistance(float minimum, float maximum) = 0;
        virtual void play(const Gek::Math::Float3 &origin, bool loop) = 0;
    };

    GEK_INTERFACE(AudioSystem)
    {
        virtual void setMasterVolume(float volume) = 0;
        virtual float getMasterVolume(void) = 0;

        virtual void setListener(const Gek::Math::Float4x4 &matrix) = 0;
        virtual void setDistanceFactor(float factor) = 0;
        virtual void setDopplerFactor(float factor) = 0;
        virtual void setRollOffFactor(float factor) = 0;

        virtual AudioEffectPtr copyEffect(AudioEffect *source) = 0;
        virtual AudioSoundPtr copySound(AudioSound *source) = 0;

        virtual AudioEffectPtr loadEffect(const wchar_t *fileName) = 0;
        virtual AudioSoundPtr loadSound(const wchar_t *fileName) = 0;
    };
}; // namespace Gek
