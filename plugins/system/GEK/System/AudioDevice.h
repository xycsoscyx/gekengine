#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Utility\Exceptions.h"
#include "GEK\Context\Context.h"

namespace Gek
{
    namespace Audio
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(CreationFailed);
        GEK_ADD_EXCEPTION(InitailizeDeviceFailed);
        GEK_ADD_EXCEPTION(LoadFileFailed);
        GEK_ADD_EXCEPTION(CreateSampleFailed);
        GEK_ADD_EXCEPTION(WriteSampleFailed);

        GEK_INTERFACE(Sample)
        {
            virtual void * const getBuffer(void) = 0;
            virtual void setFrequency(uint32_t frequency) = 0;
            virtual void setVolume(float volume) = 0;
        };

        GEK_INTERFACE(Effect)
            : public Sample
        {
            virtual void setPan(float pan) = 0;
            virtual void play(bool loop) = 0;
        };

        GEK_INTERFACE(Sound)
            : public Sample
        {
            virtual void setDistance(float minimum, float maximum) = 0;
            virtual void play(const Math::Float3 &origin, bool loop) = 0;
        };

        GEK_INTERFACE(Device)
        {
            virtual void setVolume(float volume) = 0;
            virtual float getVolume(void) = 0;

            virtual void setListener(const Math::Float4x4 &matrix) = 0;
            virtual void setDistanceFactor(float factor) = 0;
            virtual void setDopplerFactor(float factor) = 0;
            virtual void setRollOffFactor(float factor) = 0;

            virtual EffectPtr loadEffect(const wchar_t *fileName) = 0;
            virtual SoundPtr loadSound(const wchar_t *fileName) = 0;

            virtual EffectPtr copyEffect(Effect *source) = 0;
            virtual SoundPtr copySound(Sound *source) = 0;
        };
    }; // namespace Audio
}; // namespace Gek
