#include "GEK/Utility/Exceptions.hpp"
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
				GEK_REQUIRE(directSoundBuffer);

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
				GEK_REQUIRE(directSoundBuffer);

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
				GEK_REQUIRE(directSoundBuffer);
				directSoundBuffer->SetPan(uint32_t((DSBPAN_RIGHT - DSBPAN_LEFT) * pan) + DSBPAN_LEFT);
			}

			void play(bool loop)
			{
				GEK_REQUIRE(directSoundBuffer);

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
				GEK_REQUIRE(directSound8Buffer3D);

				directSound8Buffer3D->SetMinDistance(minimum, DS3D_DEFERRED);
				directSound8Buffer3D->SetMaxDistance(maximum, DS3D_DEFERRED);
			}

			void play(Math::Float3 const &origin, bool loop)
			{
				GEK_REQUIRE(directSound8Buffer3D);
				GEK_REQUIRE(directSoundBuffer);

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
				GEK_REQUIRE(window);

				GUID deviceGUID = DSDEVID_DefaultPlayback;
				if (!device.empty())
				{
					struct EnumData
					{
						std::wstring device;
						GUID *deviceGUID;
					} enumerationData = { String::Widen(String::GetLower(device)), &deviceGUID };
					DirectSoundEnumerateW([](LPGUID deviceGUID, LPCWSTR description, LPCWSTR module, void *context) -> BOOL
					{
						EnumData *enumerationData = static_cast<EnumData *>(context);
						if (enumerationData->device == String::GetLower(std::wstring(description)))
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
					throw Audio::CreationFailed("Unable to create sound controller");
				}

				resultValue = directSound->SetCooperativeLevel(window, DSSCL_PRIORITY);
				if (FAILED(resultValue))
				{
					throw Audio::InitailizeDeviceFailed("Unable to set cooperative level");
				}

				DSBUFFERDESC primaryBufferDescription = { 0 };
				primaryBufferDescription.dwSize = sizeof(DSBUFFERDESC);
				primaryBufferDescription.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_PRIMARYBUFFER;
				resultValue = directSound->CreateSoundBuffer(&primaryBufferDescription, &primarySoundBuffer, nullptr);
				if (FAILED(resultValue))
				{
					throw Audio::InitailizeDeviceFailed("Unable to create primary sound buffer");
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
					throw Audio::InitailizeDeviceFailed("Unable to set primary buffer format");
				}

				directSoundListener = primarySoundBuffer;
				if (!directSoundListener)
				{
					throw Audio::InitailizeDeviceFailed("Unable to create primary 3D listener");
				}

				setVolume(1.0f);
				setDistanceFactor(1.0f);
				setDopplerFactor(0.0f);
				setRollOffFactor(1.0f);
			}

			// Audio::Device
			void setVolume(float volume)
			{
				GEK_REQUIRE(primarySoundBuffer);

				primarySoundBuffer->SetVolume(uint32_t((DSBVOLUME_MAX - DSBVOLUME_MIN) * volume) + DSBVOLUME_MIN);
			}

			float getVolume(void)
			{
				GEK_REQUIRE(primarySoundBuffer);

				long volumeNumber = 0;
				if (FAILED(primarySoundBuffer->GetVolume(&volumeNumber)))
				{
					volumeNumber = 0;
				}

				return (float(volumeNumber - DSBVOLUME_MIN) / float(DSBVOLUME_MAX - DSBVOLUME_MIN));
			}

			void setListener(Math::Float4x4 const &matrix)
			{
				GEK_REQUIRE(directSoundListener);

				directSoundListener->SetPosition(matrix.translation.x, matrix.translation.y, matrix.translation.z, DS3D_DEFERRED);
				directSoundListener->SetOrientation(matrix.rz.x, matrix.rz.y, matrix.rz.z, matrix.ry.x, matrix.ry.y, matrix.ry.z, DS3D_DEFERRED);
				directSoundListener->CommitDeferredSettings();
			}

			void setDistanceFactor(float factor)
			{
				GEK_REQUIRE(directSoundListener);

				directSoundListener->SetDistanceFactor(factor, DS3D_DEFERRED);
			}

			void setDopplerFactor(float factor)
			{
				GEK_REQUIRE(directSoundListener);

				directSoundListener->SetDopplerFactor(factor, DS3D_DEFERRED);
			}

			void setRollOffFactor(float factor)
			{
				GEK_REQUIRE(directSoundListener);

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
				GEK_REQUIRE(directSound);

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
				GEK_REQUIRE(directSound);

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
				GEK_REQUIRE(directSound);
				GEK_REQUIRE(source);

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
				GEK_REQUIRE(directSound);
				GEK_REQUIRE(source);

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