/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK\Math\Vector4SIMD.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\Context.hpp"

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
            virtual ~Buffer(void) = default;
        };

		GEK_INTERFACE(Sound)
		{
            virtual ~Sound(void) = default;
        };

        GEK_INTERFACE(Device)
        {
            virtual ~Device(void) = default;

            virtual void setVolume(float volume) = 0;
            virtual float getVolume(void) = 0;

            virtual void setListener(const Math::SIMD::Float4x4 &matrix) = 0;

			virtual BufferPtr loadBuffer(const wchar_t *fileName) = 0;
			virtual SoundPtr createSound(void) = 0;
        };
    }; // namespace Audio
}; // namespace Gek
