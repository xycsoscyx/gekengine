/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace Audio
    {
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

            virtual void setListener(Math::Float4x4 const &matrix) = 0;

			virtual BufferPtr loadBuffer(FileSystem::Path const &filePath) = 0;
			virtual SoundPtr createSound(void) = 0;
        };
    }; // namespace Audio
}; // namespace Gek
