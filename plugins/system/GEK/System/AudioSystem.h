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
    public:
        virtual void * const getBuffer(void) = 0;
        virtual void setFrequency(UINT32 frequency) = 0;
        virtual void setVolume(float volume) = 0;
    };

    GEK_INTERFACE(AudioEffect)
        : public AudioSample
    {
    public:
        virtual void setPan(float pan) = 0;
        virtual void play(bool loop) = 0;
    };

    GEK_INTERFACE(AudioSound)
        : public AudioSample
    {
    public:
        virtual void setDistance(float minimum, float maximum) = 0;
        virtual void play(const Gek::Math::Float3 &origin, bool loop) = 0;
    };

    GEK_INTERFACE(AudioSystem)
    {
    public:
        virtual void setMasterVolume(float volume) = 0;
        virtual float getMasterVolume(void) = 0;

        virtual void setListener(const Gek::Math::Float4x4 &matrix) = 0;
        virtual void setDistanceFactor(float factor) = 0;
        virtual void setDopplerFactor(float factor) = 0;
        virtual void setRollOffFactor(float factor) = 0;

        virtual std::shared_ptr<AudioEffect> copyEffect(AudioEffect *source) = 0;
        virtual std::shared_ptr<AudioSound> copySound(AudioSound *source) = 0;

        virtual std::shared_ptr<AudioEffect> loadEffect(LPCWSTR fileName) = 0;
        virtual std::shared_ptr<AudioSound> loadSound(LPCWSTR fileName) = 0;
    };
}; // namespace Gek
