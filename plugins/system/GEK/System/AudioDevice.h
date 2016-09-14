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

		GEK_INTERFACE(Buffer)
		{
		};

		GEK_INTERFACE(Sound)
		{
		};

        GEK_INTERFACE(Device)
        {
            virtual void setVolume(float volume) = 0;
            virtual float getVolume(void) = 0;

            virtual void setListener(const Math::Float4x4 &matrix) = 0;

			virtual BufferPtr loadBuffer(const wchar_t *fileName) = 0;
			virtual SoundPtr createSound(void) = 0;
        };
    }; // namespace Audio
}; // namespace Gek
