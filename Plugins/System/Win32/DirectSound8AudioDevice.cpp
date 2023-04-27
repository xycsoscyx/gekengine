#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ContextUser.hpp"
#include "GEK/System/AudioDevice.hpp"
#include <atlbase.h>

#include <mmsystem.h>
#define DIRECTSOUND_VERSION 0x0800
#include <dsound.h>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

namespace Gek
{
	namespace DirectSound8
	{
/*
		template <typename CLASS>
		class Buffer
			: public CLASS
		{
		public:
			CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSoundBuffer;

		public:
			Buffer(IDirectSoundBuffer8 *directSoundBuffer)
				: directSoundBuffer(directSoundBuffer)
			{
				setVolume(1.0f);
			}

			virtual ~Buffer(void)
			{
			}

			// AudioSample
			void setFrequency(uint32_t frequency)
			{
				assert(directSoundBuffer);

				if (frequency == -1)
				{
					directSoundBuffer->SetFrequency(DSBFREQUENCY_ORIGINAL);
				}
				else
				{
					directSoundBuffer->SetFrequency(frequency);
				}
			}

			void setVolume(float volume)
			{
				assert(directSoundBuffer);

				directSoundBuffer->SetVolume(uint32_t((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
			}
		};

		class Effect
			: public Buffer<Audio::Effect>
		{
		public:
			Effect(IDirectSoundBuffer8 *directSoundBuffer)
				: Buffer(directSoundBuffer)
			{
			}

			// Audio::Effect
			void setPan(float pan)
			{
				assert(directSoundBuffer);
				directSoundBuffer->SetPan(uint32_t((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
			}

			void play(bool loop)
			{
				assert(directSoundBuffer);

				DWORD dwStatus = 0;
				if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
				{
					directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
				}
			}
		};

		class Sound
			: public Buffer<Audio::Sound>
		{
		public:
			CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D;

		public:
			Sound(IDirectSoundBuffer8 *directSoundBuffer, IDirectSound3DBuffer8 *directSound8Buffer3D)
				: Buffer(directSoundBuffer)
				, directSound8Buffer3D(directSound8Buffer3D)
			{
			}

			// Audio::Sound
			void setDistance(float minimum, float maximum)
			{
				assert(directSound8Buffer3D);

				directSound8Buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
				directSound8Buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
			}

			void play(Math::Float3 const &origin, bool loop)
			{
				assert(directSound8Buffer3D);
				assert(directSoundBuffer);

				directSound8Buffer3D->SetPosition(origin.x, origin.y, origin.z, DS3D_DEFERRED);

				DWORD dwStatus = 0;
				if (SUCCEEDED(directSoundBuffer->GetStatus(&dwStatus)) && !(dwStatus & DSBSTATUS_PLAYING))
				{
					directSoundBuffer->Play(0, 0, (loop ? DSBPLAY_LOOPING : 0));
				}
			}
		};
*/
		GEK_CONTEXT_USER(Device, HWND, std::string)
			, public Audio::Device
		{
		private:
			CComQIPtr<IDirectSound8, &IID_IDirectSound8> directSound;
			CComQIPtr<IDirectSound3DListener8, &IID_IDirectSound3DListener8> directSoundListener;
			CComQIPtr<IDirectSoundBuffer, &IID_IDirectSoundBuffer> primarySoundBuffer;

		public:
			Device(Context *context, HWND window, std::string device)
				: ContextRegistration(context)
			{
				assert(window);

				GUID deviceGUID = DSDEVID_DefaultPlayback;
				if (!device.empty())
				{
					struct EnumData
					{
						std::string device;
						GUID *deviceGUID;
					} enumerationData =
                    {
                        String::GetLower(device), &deviceGUID
                    };

					DirectSoundEnumerateA([](LPGUID deviceGUID, LPCSTR description, LPCSTR module, void *context) -> BOOL
					{
						EnumData *enumerationData = static_cast<EnumData *>(context);
						if (enumerationData->device == String::GetLower(description))
						{
							enumerationData->deviceGUID = deviceGUID;
							return FALSE;
						}

						return TRUE;
					}, &enumerationData);
				}

				HRESULT resultValue = DirectSoundCreate8(&deviceGUID, &directSound, nullptr);
				if (FAILED(resultValue))
				{
					throw std::exception("Unable to create sound controller");
				}

				resultValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
				if (FAILED(resultValue))
				{
					throw std::exception("Unable to set cooperative level");
				}

				DSBUFFERDESC primaryBufferDescription = { 0 };
				primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
				primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
				resultValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primarySoundBuffer, nullptr);
				if (FAILED(resultValue))
				{
					throw std::exception("Unable to create primary sound buffer");
				}

				WAVEFORMATEX primaryBufferFormat;
				primaryBufferFormat.wFormatTag = WAVE_FORMAT_PCM;
				primaryBufferFormat.nChannels = 2;
				primaryBufferFormat.wBitsPerSample = 8;
				primaryBufferFormat.nSamplesPerSec = 48000;
				primaryBufferFormat.nBlockAlign = (primaryBufferFormat.wBitsPerSample / 8 * primaryBufferFormat.nChannels);
				primaryBufferFormat.nAvgBytesPerSec = (primaryBufferFormat.nSamplesPerSec * primaryBufferFormat.nBlockAlign);
				primaryBufferFormat.cbSize = 0;
				resultValue = primarySoundBuffer->SetFormat(&primaryBufferFormat);
				if (FAILED(resultValue))
				{
					throw std::exception("Unable to set primary buffer format");
				}

				directSoundListener = primarySoundBuffer;
				if (!directSoundListener)
				{
					throw std::exception("Unable to create primary 3D listener");
				}

				setVolume(1.0f);
				setDistanceFactor(1.0f);
				setDopplerFactor(0.0f);
				setRollOffFactor(1.0f);
			}

			// Audio::Device
			void setVolume(float volume)
			{
				assert(primarySoundBuffer);

				primarySoundBuffer->SetVolume(uint32_t((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
			}

			float getVolume(void)
			{
				assert(primarySoundBuffer);

				long volumeNumber = 0;
				if (FAILED(primarySoundBuffer->GetVolume(&volumeNumber)))
				{
					volumeNumber = 0;
				}

				return (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
			}

			void setListener(Math::Float4x4 const &matrix)
			{
				assert(directSoundListener);

				directSoundListener->SetPosition(matrix.r.w.x, matrix.r.w.y, matrix.r.w.z, DS3D_DEFERRED);
				directSoundListener->SetOrientation(matrix.r.z.x, matrix.r.z.y, matrix.r.z.z, matrix.r.y.x, matrix.r.y.y, matrix.r.y.z, DS3D_DEFERRED);
				directSoundListener->CommitDeferredSettings();
			}

			void setDistanceFactor(float factor)
			{
				assert(directSoundListener);

				directSoundListener->SetDistanceFactor(factor, DS3D_DEFERRED);
			}

			void setDopplerFactor(float factor)
			{
				assert(directSoundListener);

				directSoundListener->SetDopplerFactor(factor, DS3D_DEFERRED);
			}

			void setRollOffFactor(float factor)
			{
				assert(directSoundListener);

				directSoundListener->SetRolloffFactor(factor, DS3D_DEFERRED);
			}

            Audio::BufferPtr loadBuffer(FileSystem::Path const &filePath)
            {
                return nullptr;
            }

            Audio::SoundPtr createSound(void)
            {
                return nullptr;
            }
/*
			Audio::EffectPtr loadEffect(FileSystem::Path const &filePath)
			{
				assert(directSound);

				CComPtr<IDirectSoundBuffer> directSoundBuffer(loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL));

				CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
				if (!directSound8Buffer)
				{
					throw Audio::CreateSampleFailed();
				}

				return std::make_unique<Effect>(directSound8Buffer.p);
			}

			Audio::SoundPtr loadSound(FileSystem::Path const &filePath)
			{
				assert(directSound);

				CComPtr<IDirectSoundBuffer> directSoundBuffer(loadFromFile(fileName, DSBCAPS_STATIC | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN | DSBCAPS_CTRLFREQUENCY, GUID_NULL));

				CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
				if (!directSound8Buffer)
				{
					throw Audio::CreateSampleFailed();
				}

				CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
				if (!directSound8Buffer3D)
				{
					throw Audio::CreateSampleFailed();
				}

				return std::make_unique<Sound>(directSound8Buffer.p, directSound8Buffer3D.p);
			}

			Audio::EffectPtr copyEffect(Audio::Effect *source)
			{
				assert(directSound);
				assert(source);

				CComPtr<IDirectSoundBuffer> directSoundBuffer;
				HRESULT resultValue = directSound->DuplicateSoundBuffer(dynamic_cast<Effect *>(source)->directSoundBuffer.p, &directSoundBuffer);
				if (!directSoundBuffer)
				{
					throw Audio::CreateSampleFailed();
				}

				CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
				if (!directSound8Buffer)
				{
					throw Audio::CreateSampleFailed();
				}

				return std::make_unique<Effect>(directSound8Buffer.p);
			}

			Audio::SoundPtr copySound(Audio::Sound *source)
			{
				assert(directSound);
				assert(source);

				CComPtr<IDirectSoundBuffer> directSoundBuffer;
				HRESULT resultValue = directSound->DuplicateSoundBuffer(dynamic_cast<Sound *>(source)->directSoundBuffer.p, &directSoundBuffer);
				if (!directSoundBuffer)
				{
					throw Audio::CreateSampleFailed();
				}

				CComQIPtr<IDirectSoundBuffer8, &IID_IDirectSoundBuffer8> directSound8Buffer(directSoundBuffer);
				if (!directSound8Buffer)
				{
					throw Audio::CreateSampleFailed();
				}

				CComQIPtr<IDirectSound3DBuffer8, &IID_IDirectSound3DBuffer8> directSound8Buffer3D(directSound8Buffer);
				if (!directSound8Buffer3D)
				{
					throw Audio::CreateSampleFailed();
				}

				return std::make_unique<Sound>(directSound8Buffer.p, directSound8Buffer3D.p);
			}
*/
		};

		GEK_REGISTER_CONTEXT_USER(Device);
	}; // namespace DirectSound8
}; // namespace Gek