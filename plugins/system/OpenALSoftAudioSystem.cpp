#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\FileSystem.h"
#include "GEK\Context\ContextUser.h"
#include "GEK\System\AudioDevice.h"
#include <Windows.h>

#include <AL\al.h>
#include <AL\alc.h>
#pragma comment(lib, "Winmm.lib")

namespace Gek
{
    namespace OpenALSoft
    {
		class Buffer
			: public Audio::Buffer
		{
		public:
			ALuint buffer;

		public:
			Buffer(ALuint buffer)
				: buffer(buffer)
			{
			}
		};

		class Sound
			: public Audio::Sound
		{
		public:
			ALuint source;

		public:
			Sound(ALuint source)
				: source(source)
			{
			}
		};

        GEK_CONTEXT_USER(Device, HWND, String)
            , public Audio::Device
        {
        private:
			ALCdevice *openALDevice;
			ALCcontext *openALContext;

        public:
            Device(Context *context, HWND window, String device)
                : ContextRegistration(context)
				, openALDevice(nullptr)
				, openALContext(nullptr)
            {
                throw Exception("TODO: Finish OpenALSoft Audio Device");

                GEK_REQUIRE(window);

				openALDevice = alcOpenDevice(NULL);
				if (!openALDevice)
				{
				}

				openALContext = alcCreateContext(openALDevice, NULL);
				if (!openALContext)
				{
				}

				if (!alcMakeContextCurrent(openALContext))
				{
				}

				setVolume(1.0f);
            }

			~Device(void)
			{
				if (openALContext)
				{
					alcMakeContextCurrent(NULL);
					alcDestroyContext(openALContext);
				}

				if (openALDevice)
				{
					alcCloseDevice(openALDevice);
				}
			}

            // Audio::Device
            void setVolume(float volume)
            {
            }

            float getVolume(void)
            {
				return 0.0f;
            }

            void setListener(const Math::Float4x4 &matrix)
            {
				Math::Float3 orientation[2] =
				{
					matrix.nz, matrix.ny,
				};

				alListenerfv(AL_POSITION, matrix.translation.data);
				alListenerfv(AL_ORIENTATION, orientation[0].data);
			}

			Audio::BufferPtr loadBuffer(const wchar_t *fileName)
			{
				ALuint buffer;
				alGenBuffers((ALuint)1, &buffer);

				return std::make_shared<Buffer>(buffer);
			}

			Audio::SoundPtr createSound(void)
			{
				ALuint source;
				alGenSources((ALuint)1, &source);
				// check for errors

				alSourcef(source, AL_PITCH, 1);
				// check for errors
				alSourcef(source, AL_GAIN, 1);
				// check for errors
				alSource3f(source, AL_POSITION, 0, 0, 0);
				// check for errors
				alSource3f(source, AL_VELOCITY, 0, 0, 0);
				// check for errors
				alSourcei(source, AL_LOOPING, AL_FALSE);
				// check for errros

				return std::make_shared<Sound>(source);
			}
        };

        GEK_REGISTER_CONTEXT_USER(Device);
    }; // namespace OpenALSoft
}; // namespace Gek